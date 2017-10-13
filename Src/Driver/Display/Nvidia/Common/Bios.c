#include "Bios.h"

UINT32 NvBiosPromRd32(SUBDEV_BIOS * Bios, UINT64 Offset)
{
	NVIDIA_GPU * Gpu = Bios->Parent;
	if (Offset < 0x100000)
		return NvMmioRd32(Gpu, Offset + 0x300000);

	return 0xFFFFFFFF;
}

UINT8 NvBiosRd08(SUBDEV_BIOS * Bios, UINT64 Offset)
{
	return (NvBiosPromRd32(Bios, Offset) >> ((Offset & 0x3) << 3));
}

UINT16 NvBiosRd16(SUBDEV_BIOS * Bios, UINT64 Offset)
{
	return (UINT16)NvBiosRd08(Bios, Offset) | (((UINT16)NvBiosRd08(Bios, Offset + 1) << 8));
}

UINT32 NvBiosRd32(SUBDEV_BIOS * Bios, UINT64 Offset)
{
	if (Offset % 4 == 0)
		return NvBiosPromRd32(Bios, Offset);
	else
		return NvBiosRd16(Bios, Offset) | NvBiosRd16(Bios, Offset + 2) << 16;
}

typedef struct _BIT_ENTRY
{
	UINT8 Id;
	UINT8 Version;
	UINT16 Length;
	UINT16 Offset;
}BIT_ENTRY;

enum dcb_output_type {
	DCB_OUTPUT_ANALOG = 0x0,
	DCB_OUTPUT_TV = 0x1,
	DCB_OUTPUT_TMDS = 0x2,
	DCB_OUTPUT_LVDS = 0x3,
	DCB_OUTPUT_DP = 0x6,
	DCB_OUTPUT_EOL = 0xe,
	DCB_OUTPUT_UNUSED = 0xf,
	DCB_OUTPUT_ANY = -1,
};

struct dcb_output {
	int index;	/* may not be raw dcb index if merging has happened */
	UINT16 hasht;
	UINT16 hashm;
	enum dcb_output_type type;
	UINT8 i2c_index;
	UINT8 heads;
	UINT8 connector;
	UINT8 bus;
	UINT8 location;
	UINT8 or ;
	UINT8 link;
	BOOLEAN duallink_possible;
	UINT8 extdev;
	union {
		struct sor_conf {
			int link;
		} sorconf;
		struct {
			int maxfreq;
		} crtconf;
		struct {
			struct sor_conf sor;
			BOOLEAN use_straps_for_mode;
			BOOLEAN use_acpi_for_edid;
			BOOLEAN use_power_scripts;
		} lvdsconf;
		struct {
			BOOLEAN has_component_output;
		} tvconf;
		struct {
			struct sor_conf sor;
			int link_nr;
			int link_bw;
		} dpconf;
		struct {
			struct sor_conf sor;
			int slave_addr;
		} tmdsconf;
	};
	BOOLEAN i2c_upper_default;
};

RETURN_STATUS BitEntry(UINT8 * Bios, UINT32 BitStrOffset, UINT8 Id, BIT_ENTRY * Bit)
{
	UINT8 BitEntries = Bios[BitStrOffset + 10];
	UINT32 Entry = BitStrOffset + 12;

	while (BitEntries--)
	{
		if (Bios[Entry + 0] == Id)
		{
			Bit->Id = Bios[Entry + 0];
			Bit->Version = Bios[Entry + 1];
			Bit->Length = Bios[Entry + 2];
			Bit->Offset = (Bios[Entry + 5] << 8) | Bios[Entry + 4];
			return RETURN_SUCCESS;
		}
		Entry += Bios[BitStrOffset + 9];
	}
	return RETURN_ABORTED;
};


//UINT64 NvBiosRammapTe(UINT8 * BiosData)
//{
//	UINT16 RamMap = 0;
//	UINT8 *BitString = "\xff\xb8""BIT";
//	UINT64 BitStrOffset = 0;
//	BIT_ENTRY BitP;
//
//	int i;
//	for (i = 0; i < BiosSize; i++)
//	{
//		if (!memcmp(BitString, BiosPtr, 5))
//			break;
//		BiosPtr++;
//	}
//
//	if (!BitEntry(BiosData, 'P', &BitP))
//	{
//		if (BitP.Version == 2)
//		{
//			RamMap = BiosData[BitP.Offset + 4];
//		}
//	}
//	return RamMap;
//}


UINT16 DcbTable(SUBDEV_BIOS * Bios, UINT8 * Ver, UINT8 * Hdr, UINT8 * Cnt, UINT8 *Len)
{
	UINT16 Dcb = 0;
	Dcb = NvBiosRd16(Bios, 0x36);
	NVIDIA_GPU * Gpu = Bios->Parent;

	*Ver = NvBiosRd08(Bios, Dcb);

	if (*Ver >= 0x42)
	{
		KDEBUG("DCB version 0x%02x unknown\n", *Ver);
		return 0;
	}

	if (*Ver >= 0x30)
	{
		if (NvBiosRd32(Bios, Dcb + 6) == 0x4edcbdcb)
		{
			*Hdr = NvBiosRd08(Bios, Dcb + 1);
			*Cnt = NvBiosRd08(Bios, Dcb + 2);
			*Len = NvBiosRd08(Bios, Dcb + 3);
			return Dcb;
		}
	}
	if (*Ver >= 0x20)
	{
		if (NvBiosRd32(Bios, Dcb + 4) == 0x4edcbdcb)
		{
			UINT16 I2c = NvBiosRd16(Bios, Dcb + 2);
			*Hdr = 8;
			*Cnt = (I2c - Dcb) / 8;
			*Len = 8;
			return Dcb;
		}
	}
	if (*Ver >= 0x15)
	{
		if (!memcmp(&Bios->BiosData[Dcb - 7], "DEV_REC", 7))
		{
			UINT16 I2c = NvBiosRd16(Bios, Dcb + 2);
			*Hdr = 4;
			*Cnt = (I2c - Dcb) / 10;
			*Len = 10;
			return Dcb;
		}
	}
	KDEBUG("DCB contains no userful data\n");

}

UINT16
DcbOutp(SUBDEV_BIOS * Bios, UINT8 idx, UINT8 *ver, UINT8 *len)
{
	UINT8  hdr, cnt;
	UINT16 dcb = DcbTable(Bios, ver, &hdr, &cnt, len);
	if (dcb && idx < cnt)
		return dcb + hdr + (idx * *len);
	return 0x0000;
}

static inline UINT16
DcbOutpHashT(struct dcb_output *Outp)
{
	return (Outp->extdev << 8) | (Outp->location << 4) | Outp->type;
}

static inline UINT16
DcbOutpHashM(struct dcb_output *Outp)
{
	return (Outp->heads << 8) | (Outp->link << 6) | Outp-> or ;
}

UINT16 DcbOutpParse(SUBDEV_BIOS * Bios, UINT8 Idx, UINT8 * Ver, UINT8 * Len, struct dcb_output *Outp)
{
	UINT16 dcb = DcbOutp(Bios, Idx, Ver, Len);
	memset(Outp, 0x00, sizeof(*Outp));
	
	if (dcb) {
		if (*Ver >= 0x20) {
			UINT32 conn = NvBiosRd32(Bios, dcb + 0x00);
			Outp-> or = (conn & 0x0f000000) >> 24;
			Outp->location = (conn & 0x00300000) >> 20;
			Outp->bus = (conn & 0x000f0000) >> 16;
			Outp->connector = (conn & 0x0000f000) >> 12;
			Outp->heads = (conn & 0x00000f00) >> 8;
			Outp->i2c_index = (conn & 0x000000f0) >> 4;
			Outp->type = (conn & 0x0000000f);
			Outp->link = 0;
		}
		else {
			dcb = 0x0000;
		}

		if (*Ver >= 0x40) {
			UINT32 conf = NvBiosRd32(Bios, dcb + 0x04);
			switch (Outp->type) {
			case DCB_OUTPUT_DP:
				switch (conf & 0x00e00000) {
				case 0x00000000: /* 1.62 */
					Outp->dpconf.link_bw = 0x06;
					break;
				case 0x00200000: /* 2.7 */
					Outp->dpconf.link_bw = 0x0a;
					break;
				case 0x00400000: /* 5.4 */
					Outp->dpconf.link_bw = 0x14;
					break;
				case 0x00600000: /* 8.1 */
				default:
					Outp->dpconf.link_bw = 0x1e;
					break;
				}

				switch ((conf & 0x0f000000) >> 24) {
				case 0xf:
				case 0x4:
					Outp->dpconf.link_nr = 4;
					break;
				case 0x3:
				case 0x2:
					Outp->dpconf.link_nr = 2;
					break;
				case 0x1:
				default:
					Outp->dpconf.link_nr = 1;
					break;
				}

				/* fall-through... */
			case DCB_OUTPUT_TMDS:
			case DCB_OUTPUT_LVDS:
				Outp->link = (conf & 0x00000030) >> 4;
				Outp->sorconf.link = Outp->link; /*XXX*/
				Outp->extdev = 0x00;
				if (Outp->location != 0)
					Outp->extdev = (conf & 0x0000ff00) >> 8;
				break;
			default:
				break;
			}
		}

		Outp->hasht = DcbOutpHashT(Outp);
		Outp->hashm = DcbOutpHashM(Outp);
	}
	return dcb;
}


enum dcb_connector_type {
	DCB_CONNECTOR_VGA = 0x00,
	DCB_CONNECTOR_TV_0 = 0x10,
	DCB_CONNECTOR_TV_1 = 0x11,
	DCB_CONNECTOR_TV_3 = 0x13,
	DCB_CONNECTOR_DVI_I = 0x30,
	DCB_CONNECTOR_DVI_D = 0x31,
	DCB_CONNECTOR_DMS59_0 = 0x38,
	DCB_CONNECTOR_DMS59_1 = 0x39,
	DCB_CONNECTOR_LVDS = 0x40,
	DCB_CONNECTOR_LVDS_SPWG = 0x41,
	DCB_CONNECTOR_DP = 0x46,
	DCB_CONNECTOR_eDP = 0x47,
	DCB_CONNECTOR_HDMI_0 = 0x60,
	DCB_CONNECTOR_HDMI_1 = 0x61,
	DCB_CONNECTOR_HDMI_C = 0x63,
	DCB_CONNECTOR_DMS59_DP0 = 0x64,
	DCB_CONNECTOR_DMS59_DP1 = 0x65,
	DCB_CONNECTOR_NONE = 0xff
};

struct nvBios_connT {
};

struct nvBios_connE {
	UINT8 type;
	UINT8 location;
	UINT8 hpd;
	UINT8 dp;
	UINT8 di;
	UINT8 sr;
	UINT8 lcdid;
};

UINT32
nvBios_connTe(SUBDEV_BIOS *Bios, UINT8 *ver, UINT8 *hdr, UINT8 *cnt, UINT8 *len)
{
	UINT32 dcb = DcbTable(Bios, ver, hdr, cnt, len);
	if (dcb && *ver >= 0x30 && *hdr >= 0x16) {
		UINT32 data = NvBiosRd16(Bios, dcb + 0x14);
		if (data) {
			*ver = NvBiosRd08(Bios, data + 0);
			*hdr = NvBiosRd08(Bios, data + 1);
			*cnt = NvBiosRd08(Bios, data + 2);
			*len = NvBiosRd08(Bios, data + 3);
			return data;
		}
	}
	return 0x00000000;
}

UINT32
nvBios_connTp(SUBDEV_BIOS *Bios, UINT8 *ver, UINT8 *hdr, UINT8 *cnt, UINT8 *len,
struct nvBios_connT *info)
{
	UINT32 data = nvBios_connTe(Bios, ver, hdr, cnt, len);
	memset(info, 0x00, sizeof(*info));
	switch (!!data * *ver) {
	case 0x30:
	case 0x40:
		return data;
	default:
		break;
	}
	return 0x00000000;
}

UINT32
nvBios_connEe(SUBDEV_BIOS *Bios, UINT8 idx, UINT8 *ver, UINT8 *len)
{
	UINT8  hdr, cnt;
	UINT32 data = nvBios_connTe(Bios, ver, &hdr, &cnt, len);
	if (data && idx < cnt)
		return data + hdr + (idx * *len);
	return 0x00000000;
}

UINT32
nvBios_connEp(SUBDEV_BIOS *Bios, UINT8 idx, UINT8 *ver, UINT8 *len,
struct nvBios_connE *info)
{
	UINT32 data = nvBios_connEe(Bios, idx, ver, len);
	memset(info, 0x00, sizeof(*info));
	switch (!!data * *ver) {
	case 0x30:
	case 0x40:
		info->type = NvBiosRd08(Bios, data + 0x00);
		info->location = NvBiosRd08(Bios, data + 0x01) & 0x0f;
		info->hpd = (NvBiosRd08(Bios, data + 0x01) & 0x30) >> 4;
		info->dp = (NvBiosRd08(Bios, data + 0x01) & 0xc0) >> 6;
		if (*len < 4)
			return data;
		info->hpd |= (NvBiosRd08(Bios, data + 0x02) & 0x03) << 2;
		info->dp |= NvBiosRd08(Bios, data + 0x02) & 0x0c;
		info->di = (NvBiosRd08(Bios, data + 0x02) & 0xf0) >> 4;
		info->hpd |= (NvBiosRd08(Bios, data + 0x03) & 0x07) << 4;
		info->sr = (NvBiosRd08(Bios, data + 0x03) & 0x08) >> 3;
		info->lcdid = (NvBiosRd08(Bios, data + 0x03) & 0x70) >> 4;
		return data;
	default:
		break;
	}
	return 0x00000000;
}



RETURN_STATUS NvidiaBiosInit(SUBDEV_BIOS * Bios)
{
	NVIDIA_GPU * Gpu = Bios->Parent;
	UINT8 *BitString = "\xff\xb8""BIT";
	UINT8 *BmpString = "\xff\x7f""NV\0";
	BIT_ENTRY VersionBitEntry;

	UINT8 *BiosSignature = "HWSQ";

	Bios->BiosData = KMalloc(0x100000);
	int i;
	for (i = 0; i < 0x100000; i+=4)
	{
		*(UINT32 *)(Bios->BiosData + i) = NvBiosRd32(Bios, i);
	}
	
	UINT64 BiosSize = NvBiosRd08(Bios, 2) * 512;
	UINT8 * BiosPtr = &Bios->BiosData[0];
	UINT64 BitStrOffset = 0;
	UINT64 BmpStringOffset = 0;

	KDEBUG("NVIDIA:VBIOS Size = %dKB\n", BiosSize / 1024);

	for (i = 0; i < BiosSize; i++)
	{
		if (!memcmp(BitString, BiosPtr, 5))
			break;
		BiosPtr++;
	}

	if (i == BiosSize)
	{
		KDEBUG("BIT not found.\n");
	}
	else
	{
		BitStrOffset = BiosPtr - &Bios->BiosData[0];
	}

	BitEntry(Bios->BiosData, BitStrOffset, 'i', &VersionBitEntry);
	KDEBUG("NVIDIA:VBIOS Version: %02x.%02x.%02x.%02x.%02x\n",
		NvBiosRd08(Bios, VersionBitEntry.Offset + 3),
		NvBiosRd08(Bios, VersionBitEntry.Offset + 2),
		NvBiosRd08(Bios, VersionBitEntry.Offset + 1),
		NvBiosRd08(Bios, VersionBitEntry.Offset + 0),
		NvBiosRd08(Bios, VersionBitEntry.Offset + 4)
		);
	i = 0;
	struct dcb_output DcbE;
	UINT8 Ver, Head;
	KDEBUG("Connectors:\n");
	char * ConnType[] =
	{
		"Analog",
		"TV",
		"TMDS",
		"LVDS",
		"",
		"",
		"DP",
		"EOL",
		"Unused"
	};
	while (DcbOutpParse(Bios, i++, &Ver, &Head, &DcbE))
	{
		switch (DcbE.type)
		{
			case DCB_OUTPUT_ANALOG:
			case DCB_OUTPUT_TV:
			case DCB_OUTPUT_TMDS:
			case DCB_OUTPUT_LVDS:
			case DCB_OUTPUT_DP:
				KDEBUG("%d.Location:%s, Type:%s\n", i, DcbE.location ? "External" : "Internal", ConnType[DcbE.type]);
				break;
		default:
			continue;
		}
		
	}
	return RETURN_SUCCESS;
}
