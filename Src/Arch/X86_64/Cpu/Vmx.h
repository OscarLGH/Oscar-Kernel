#ifndef VMX_H
#define VMX_H

#include "Vm.h"
#include "Cpu.h"

/*
 * Definitions of Primary Processor-Based VM-Execution Controls.
 */
#define CPU_BASED_VIRTUAL_INTR_PENDING          0x00000004
#define CPU_BASED_USE_TSC_OFFSETING             0x00000008
#define CPU_BASED_HLT_EXITING                   0x00000080
#define CPU_BASED_INVLPG_EXITING                0x00000200
#define CPU_BASED_MWAIT_EXITING                 0x00000400
#define CPU_BASED_RDPMC_EXITING                 0x00000800
#define CPU_BASED_RDTSC_EXITING                 0x00001000
#define CPU_BASED_CR3_LOAD_EXITING				0x00008000
#define CPU_BASED_CR3_STORE_EXITING				0x00010000
#define CPU_BASED_CR8_LOAD_EXITING              0x00080000
#define CPU_BASED_CR8_STORE_EXITING             0x00100000
#define CPU_BASED_TPR_SHADOW                    0x00200000
#define CPU_BASED_VIRTUAL_NMI_PENDING			0x00400000
#define CPU_BASED_MOV_DR_EXITING                0x00800000
#define CPU_BASED_UNCOND_IO_EXITING             0x01000000
#define CPU_BASED_USE_IO_BITMAPS                0x02000000
#define CPU_BASED_MONITOR_TRAP_FLAG             0x08000000
#define CPU_BASED_USE_MSR_BITMAPS               0x10000000
#define CPU_BASED_MONITOR_EXITING               0x20000000
#define CPU_BASED_PAUSE_EXITING                 0x40000000
#define CPU_BASED_ACTIVATE_SECONDARY_CONTROLS   0x80000000

#define CPU_BASED_ALWAYSON_WITHOUT_TRUE_MSR		0x0401e172

#define CPU_BASED_FIXED_ONES					0x04006172
#define CPU_BASED_FIXED_ZEROS					0xfff9fffe
/*
 * Definitions of Secondary Processor-Based VM-Execution Controls.
 */
#define SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES 0x00000001
#define SECONDARY_EXEC_ENABLE_EPT               0x00000002
#define SECONDARY_EXEC_DESC						0x00000004
#define SECONDARY_EXEC_RDTSCP					0x00000008
#define SECONDARY_EXEC_VIRTUALIZE_X2APIC_MODE   0x00000010
#define SECONDARY_EXEC_ENABLE_VPID              0x00000020
#define SECONDARY_EXEC_WBINVD_EXITING			0x00000040
#define SECONDARY_EXEC_UNRESTRICTED_GUEST		0x00000080
#define SECONDARY_EXEC_APIC_REGISTER_VIRT       0x00000100
#define SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY    0x00000200
#define SECONDARY_EXEC_PAUSE_LOOP_EXITING		0x00000400
#define SECONDARY_EXEC_RDRAND					0x00000800
#define SECONDARY_EXEC_ENABLE_INVPCID			0x00001000
#define SECONDARY_EXEC_SHADOW_VMCS              0x00004000
#define SECONDARY_EXEC_RDSEED					0x00010000
#define SECONDARY_EXEC_ENABLE_PML               0x00020000
#define SECONDARY_EXEC_XSAVES					0x00100000
#define SECONDARY_EXEC_TSC_SCALING              0x02000000

#define PIN_BASED_EXT_INTR_MASK                 0x00000001
#define PIN_BASED_NMI_EXITING                   0x00000008
#define PIN_BASED_VIRTUAL_NMIS                  0x00000020
#define PIN_BASED_VMX_PREEMPTION_TIMER          0x00000040
#define PIN_BASED_POSTED_INTR                   0x00000080

#define PIN_BASED_ALWAYSON_WITHOUT_TRUE_MSR		0x00000016

#define VM_EXIT_SAVE_DEBUG_CONTROLS             0x00000004
#define VM_EXIT_HOST_ADDR_SPACE_SIZE            0x00000200
#define VM_EXIT_LOAD_IA32_PERF_GLOBAL_CTRL      0x00001000
#define VM_EXIT_ACK_INTR_ON_EXIT                0x00008000
#define VM_EXIT_SAVE_IA32_PAT					0x00040000
#define VM_EXIT_LOAD_IA32_PAT					0x00080000
#define VM_EXIT_SAVE_IA32_EFER                  0x00100000
#define VM_EXIT_LOAD_IA32_EFER                  0x00200000
#define VM_EXIT_SAVE_VMX_PREEMPTION_TIMER       0x00400000
#define VM_EXIT_CLEAR_BNDCFGS                   0x00800000

#define VM_EXIT_ALWAYSON_WITHOUT_TRUE_MSR		0x00036fff

#define VM_ENTRY_LOAD_DEBUG_CONTROLS            0x00000004
#define VM_ENTRY_IA32E_MODE                     0x00000200
#define VM_ENTRY_SMM                            0x00000400
#define VM_ENTRY_DEACT_DUAL_MONITOR             0x00000800
#define VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL     0x00002000
#define VM_ENTRY_LOAD_IA32_PAT					0x00004000
#define VM_ENTRY_LOAD_IA32_EFER                 0x00008000
#define VM_ENTRY_LOAD_BNDCFGS                   0x00010000

#define VM_ENTRY_ALWAYSON_WITHOUT_TRUE_MSR		0x000011ff

#define VMX_MISC_PREEMPTION_TIMER_RATE_MASK		0x0000001f
#define VMX_MISC_SAVE_EFER_LMA					0x00000020
#define VMX_MISC_ACTIVITY_HLT					0x00000040

enum vmcs_field {
	VIRTUAL_PROCESSOR_ID            = 0x00000000,
	POSTED_INTR_NV                  = 0x00000002,
	GUEST_ES_SELECTOR               = 0x00000800,
	GUEST_CS_SELECTOR               = 0x00000802,
	GUEST_SS_SELECTOR               = 0x00000804,
	GUEST_DS_SELECTOR               = 0x00000806,
	GUEST_FS_SELECTOR               = 0x00000808,
	GUEST_GS_SELECTOR               = 0x0000080a,
	GUEST_LDTR_SELECTOR             = 0x0000080c,
	GUEST_TR_SELECTOR               = 0x0000080e,
	GUEST_INTR_STATUS               = 0x00000810,
	GUEST_PML_INDEX			= 0x00000812,
	HOST_ES_SELECTOR                = 0x00000c00,
	HOST_CS_SELECTOR                = 0x00000c02,
	HOST_SS_SELECTOR                = 0x00000c04,
	HOST_DS_SELECTOR                = 0x00000c06,
	HOST_FS_SELECTOR                = 0x00000c08,
	HOST_GS_SELECTOR                = 0x00000c0a,
	HOST_TR_SELECTOR                = 0x00000c0c,
	IO_BITMAP_A                     = 0x00002000,
	IO_BITMAP_A_HIGH                = 0x00002001,
	IO_BITMAP_B                     = 0x00002002,
	IO_BITMAP_B_HIGH                = 0x00002003,
	MSR_BITMAP                      = 0x00002004,
	MSR_BITMAP_HIGH                 = 0x00002005,
	VM_EXIT_MSR_STORE_ADDR          = 0x00002006,
	VM_EXIT_MSR_STORE_ADDR_HIGH     = 0x00002007,
	VM_EXIT_MSR_LOAD_ADDR           = 0x00002008,
	VM_EXIT_MSR_LOAD_ADDR_HIGH      = 0x00002009,
	VM_ENTRY_MSR_LOAD_ADDR          = 0x0000200a,
	VM_ENTRY_MSR_LOAD_ADDR_HIGH     = 0x0000200b,
	PML_ADDRESS			= 0x0000200e,
	PML_ADDRESS_HIGH		= 0x0000200f,
	TSC_OFFSET                      = 0x00002010,
	TSC_OFFSET_HIGH                 = 0x00002011,
	VIRTUAL_APIC_PAGE_ADDR          = 0x00002012,
	VIRTUAL_APIC_PAGE_ADDR_HIGH     = 0x00002013,
	APIC_ACCESS_ADDR		= 0x00002014,
	APIC_ACCESS_ADDR_HIGH		= 0x00002015,
	POSTED_INTR_DESC_ADDR           = 0x00002016,
	POSTED_INTR_DESC_ADDR_HIGH      = 0x00002017,
	EPT_POINTER                     = 0x0000201a,
	EPT_POINTER_HIGH                = 0x0000201b,
	EOI_EXIT_BITMAP0                = 0x0000201c,
	EOI_EXIT_BITMAP0_HIGH           = 0x0000201d,
	EOI_EXIT_BITMAP1                = 0x0000201e,
	EOI_EXIT_BITMAP1_HIGH           = 0x0000201f,
	EOI_EXIT_BITMAP2                = 0x00002020,
	EOI_EXIT_BITMAP2_HIGH           = 0x00002021,
	EOI_EXIT_BITMAP3                = 0x00002022,
	EOI_EXIT_BITMAP3_HIGH           = 0x00002023,
	VMREAD_BITMAP                   = 0x00002026,
	VMWRITE_BITMAP                  = 0x00002028,
	XSS_EXIT_BITMAP                 = 0x0000202C,
	XSS_EXIT_BITMAP_HIGH            = 0x0000202D,
	TSC_MULTIPLIER                  = 0x00002032,
	TSC_MULTIPLIER_HIGH             = 0x00002033,
	GUEST_PHYSICAL_ADDRESS          = 0x00002400,
	GUEST_PHYSICAL_ADDRESS_HIGH     = 0x00002401,
	VMCS_LINK_POINTER               = 0x00002800,
	VMCS_LINK_POINTER_HIGH          = 0x00002801,
	GUEST_IA32_DEBUGCTL             = 0x00002802,
	GUEST_IA32_DEBUGCTL_HIGH        = 0x00002803,
	GUEST_IA32_PAT			= 0x00002804,
	GUEST_IA32_PAT_HIGH		= 0x00002805,
	GUEST_IA32_EFER			= 0x00002806,
	GUEST_IA32_EFER_HIGH		= 0x00002807,
	GUEST_IA32_PERF_GLOBAL_CTRL	= 0x00002808,
	GUEST_IA32_PERF_GLOBAL_CTRL_HIGH= 0x00002809,
	GUEST_PDPTR0                    = 0x0000280a,
	GUEST_PDPTR0_HIGH               = 0x0000280b,
	GUEST_PDPTR1                    = 0x0000280c,
	GUEST_PDPTR1_HIGH               = 0x0000280d,
	GUEST_PDPTR2                    = 0x0000280e,
	GUEST_PDPTR2_HIGH               = 0x0000280f,
	GUEST_PDPTR3                    = 0x00002810,
	GUEST_PDPTR3_HIGH               = 0x00002811,
	GUEST_BNDCFGS                   = 0x00002812,
	GUEST_BNDCFGS_HIGH              = 0x00002813,
	HOST_IA32_PAT			= 0x00002c00,
	HOST_IA32_PAT_HIGH		= 0x00002c01,
	HOST_IA32_EFER			= 0x00002c02,
	HOST_IA32_EFER_HIGH		= 0x00002c03,
	HOST_IA32_PERF_GLOBAL_CTRL	= 0x00002c04,
	HOST_IA32_PERF_GLOBAL_CTRL_HIGH	= 0x00002c05,
	PIN_BASED_VM_EXEC_CONTROL       = 0x00004000,
	CPU_BASED_VM_EXEC_CONTROL       = 0x00004002,
	EXCEPTION_BITMAP                = 0x00004004,
	PAGE_FAULT_ERROR_CODE_MASK      = 0x00004006,
	PAGE_FAULT_ERROR_CODE_MATCH     = 0x00004008,
	CR3_TARGET_COUNT                = 0x0000400a,
	VM_EXIT_CONTROLS                = 0x0000400c,
	VM_EXIT_MSR_STORE_COUNT         = 0x0000400e,
	VM_EXIT_MSR_LOAD_COUNT          = 0x00004010,
	VM_ENTRY_CONTROLS               = 0x00004012,
	VM_ENTRY_MSR_LOAD_COUNT         = 0x00004014,
	VM_ENTRY_INTR_INFO_FIELD        = 0x00004016,
	VM_ENTRY_EXCEPTION_ERROR_CODE   = 0x00004018,
	VM_ENTRY_INSTRUCTION_LEN        = 0x0000401a,
	TPR_THRESHOLD                   = 0x0000401c,
	SECONDARY_VM_EXEC_CONTROL       = 0x0000401e,
	PLE_GAP                         = 0x00004020,
	PLE_WINDOW                      = 0x00004022,
	VM_INSTRUCTION_ERROR            = 0x00004400,
	VM_EXIT_REASON                  = 0x00004402,
	VM_EXIT_INTR_INFO               = 0x00004404,
	VM_EXIT_INTR_ERROR_CODE         = 0x00004406,
	IDT_VECTORING_INFO_FIELD        = 0x00004408,
	IDT_VECTORING_ERROR_CODE        = 0x0000440a,
	VM_EXIT_INSTRUCTION_LEN         = 0x0000440c,
	VMX_INSTRUCTION_INFO            = 0x0000440e,
	GUEST_ES_LIMIT                  = 0x00004800,
	GUEST_CS_LIMIT                  = 0x00004802,
	GUEST_SS_LIMIT                  = 0x00004804,
	GUEST_DS_LIMIT                  = 0x00004806,
	GUEST_FS_LIMIT                  = 0x00004808,
	GUEST_GS_LIMIT                  = 0x0000480a,
	GUEST_LDTR_LIMIT                = 0x0000480c,
	GUEST_TR_LIMIT                  = 0x0000480e,
	GUEST_GDTR_LIMIT                = 0x00004810,
	GUEST_IDTR_LIMIT                = 0x00004812,
	GUEST_ES_AR_BYTES               = 0x00004814,
	GUEST_CS_AR_BYTES               = 0x00004816,
	GUEST_SS_AR_BYTES               = 0x00004818,
	GUEST_DS_AR_BYTES               = 0x0000481a,
	GUEST_FS_AR_BYTES               = 0x0000481c,
	GUEST_GS_AR_BYTES               = 0x0000481e,
	GUEST_LDTR_AR_BYTES             = 0x00004820,
	GUEST_TR_AR_BYTES               = 0x00004822,
	GUEST_INTERRUPTIBILITY_INFO     = 0x00004824,
	GUEST_ACTIVITY_STATE            = 0X00004826,
	GUEST_SYSENTER_CS               = 0x0000482A,
	VMX_PREEMPTION_TIMER_VALUE      = 0x0000482E,
	HOST_IA32_SYSENTER_CS           = 0x00004c00,
	CR0_GUEST_HOST_MASK             = 0x00006000,
	CR4_GUEST_HOST_MASK             = 0x00006002,
	CR0_READ_SHADOW                 = 0x00006004,
	CR4_READ_SHADOW                 = 0x00006006,
	CR3_TARGET_VALUE0               = 0x00006008,
	CR3_TARGET_VALUE1               = 0x0000600a,
	CR3_TARGET_VALUE2               = 0x0000600c,
	CR3_TARGET_VALUE3               = 0x0000600e,
	EXIT_QUALIFICATION              = 0x00006400,
	GUEST_LINEAR_ADDRESS            = 0x0000640a,
	GUEST_CR0                       = 0x00006800,
	GUEST_CR3                       = 0x00006802,
	GUEST_CR4                       = 0x00006804,
	GUEST_ES_BASE                   = 0x00006806,
	GUEST_CS_BASE                   = 0x00006808,
	GUEST_SS_BASE                   = 0x0000680a,
	GUEST_DS_BASE                   = 0x0000680c,
	GUEST_FS_BASE                   = 0x0000680e,
	GUEST_GS_BASE                   = 0x00006810,
	GUEST_LDTR_BASE                 = 0x00006812,
	GUEST_TR_BASE                   = 0x00006814,
	GUEST_GDTR_BASE                 = 0x00006816,
	GUEST_IDTR_BASE                 = 0x00006818,
	GUEST_DR7                       = 0x0000681a,
	GUEST_RSP                       = 0x0000681c,
	GUEST_RIP                       = 0x0000681e,
	GUEST_RFLAGS                    = 0x00006820,
	GUEST_PENDING_DBG_EXCEPTIONS    = 0x00006822,
	GUEST_SYSENTER_ESP              = 0x00006824,
	GUEST_SYSENTER_EIP              = 0x00006826,
	HOST_CR0                        = 0x00006c00,
	HOST_CR3                        = 0x00006c02,
	HOST_CR4                        = 0x00006c04,
	HOST_FS_BASE                    = 0x00006c06,
	HOST_GS_BASE                    = 0x00006c08,
	HOST_TR_BASE                    = 0x00006c0a,
	HOST_GDTR_BASE                  = 0x00006c0c,
	HOST_IDTR_BASE                  = 0x00006c0e,
	HOST_IA32_SYSENTER_ESP          = 0x00006c10,
	HOST_IA32_SYSENTER_EIP          = 0x00006c12,
	HOST_RSP                        = 0x00006c14,
	HOST_RIP                        = 0x00006c16,
};


/*
 * Interruption-information format
 */
#define INTR_INFO_VECTOR_MASK           0xff            /* 7:0 */
#define INTR_INFO_INTR_TYPE_MASK        0x700           /* 10:8 */
#define INTR_INFO_DELIVER_CODE_MASK     0x800           /* 11 */
#define INTR_INFO_UNBLOCK_NMI		0x1000		/* 12 */
#define INTR_INFO_VALID_MASK            0x80000000      /* 31 */
#define INTR_INFO_RESVD_BITS_MASK       0x7ffff000

#define VECTORING_INFO_VECTOR_MASK           	INTR_INFO_VECTOR_MASK
#define VECTORING_INFO_TYPE_MASK        	INTR_INFO_INTR_TYPE_MASK
#define VECTORING_INFO_DELIVER_CODE_MASK    	INTR_INFO_DELIVER_CODE_MASK
#define VECTORING_INFO_VALID_MASK       	INTR_INFO_VALID_MASK

#define INTR_TYPE_EXT_INTR              (0 << 8) /* external interrupt */
#define INTR_TYPE_NMI_INTR		(2 << 8) /* NMI */
#define INTR_TYPE_HARD_EXCEPTION	(3 << 8) /* processor exception */
#define INTR_TYPE_SOFT_INTR             (4 << 8) /* software interrupt */
#define INTR_TYPE_SOFT_EXCEPTION	(6 << 8) /* software exception */

/* GUEST_INTERRUPTIBILITY_INFO flags. */
#define GUEST_INTR_STATE_STI		0x00000001
#define GUEST_INTR_STATE_MOV_SS		0x00000002
#define GUEST_INTR_STATE_SMI		0x00000004
#define GUEST_INTR_STATE_NMI		0x00000008

/* GUEST_ACTIVITY_STATE flags */
#define GUEST_ACTIVITY_ACTIVE		0
#define GUEST_ACTIVITY_HLT		1
#define GUEST_ACTIVITY_SHUTDOWN		2
#define GUEST_ACTIVITY_WAIT_SIPI	3

/*
 * Exit Qualifications for MOV for Control Register Access
 */
#define CONTROL_REG_ACCESS_NUM          0x7     /* 2:0, number of control reg.*/
#define CONTROL_REG_ACCESS_TYPE         0x30    /* 5:4, access type */
#define CONTROL_REG_ACCESS_REG          0xf00   /* 10:8, general purpose reg. */
#define LMSW_SOURCE_DATA_SHIFT 16
#define LMSW_SOURCE_DATA  (0xFFFF << LMSW_SOURCE_DATA_SHIFT) /* 16:31 lmsw source */
#define REG_EAX                         (0 << 8)
#define REG_ECX                         (1 << 8)
#define REG_EDX                         (2 << 8)
#define REG_EBX                         (3 << 8)
#define REG_ESP                         (4 << 8)
#define REG_EBP                         (5 << 8)
#define REG_ESI                         (6 << 8)
#define REG_EDI                         (7 << 8)
#define REG_R8                         (8 << 8)
#define REG_R9                         (9 << 8)
#define REG_R10                        (10 << 8)
#define REG_R11                        (11 << 8)
#define REG_R12                        (12 << 8)
#define REG_R13                        (13 << 8)
#define REG_R14                        (14 << 8)
#define REG_R15                        (15 << 8)

/*
 * Exit Qualifications for MOV for Debug Register Access
 */
#define DEBUG_REG_ACCESS_NUM            0x7     /* 2:0, number of debug reg. */
#define DEBUG_REG_ACCESS_TYPE           0x10    /* 4, direction of access */
#define TYPE_MOV_TO_DR                  (0 << 4)
#define TYPE_MOV_FROM_DR                (1 << 4)
#define DEBUG_REG_ACCESS_REG(eq)        (((eq) >> 8) & 0xf) /* 11:8, general purpose reg. */


/*
 * Exit Qualifications for APIC-Access
 */
#define APIC_ACCESS_OFFSET              0xfff   /* 11:0, offset within the APIC page */
#define APIC_ACCESS_TYPE                0xf000  /* 15:12, access type */
#define TYPE_LINEAR_APIC_INST_READ      (0 << 12)
#define TYPE_LINEAR_APIC_INST_WRITE     (1 << 12)
#define TYPE_LINEAR_APIC_INST_FETCH     (2 << 12)
#define TYPE_LINEAR_APIC_EVENT          (3 << 12)
#define TYPE_PHYSICAL_APIC_EVENT        (10 << 12)
#define TYPE_PHYSICAL_APIC_INST         (15 << 12)

/* segment AR in VMCS -- these are different from what LAR reports */
#define VMX_SEGMENT_AR_L_MASK (1 << 13)

#define VMX_AR_TYPE_ACCESSES_MASK 1
#define VMX_AR_TYPE_READABLE_CODE_MASK (1 << 1)
#define VMX_AR_TYPE_WRITABLE_DATA_MASK (1 << 1)
#define VMX_AR_TYPE_CONFORMING_MASK (1 << 2)
#define VMX_AR_TYPE_EXPAND_DOWN_MASK (1 << 2)
#define VMX_AR_TYPE_CODE_MASK (1 << 3)
#define VMX_AR_TYPE_MASK 0x0f
#define VMX_AR_TYPE_BUSY_64_TSS 11
#define VMX_AR_TYPE_BUSY_32_TSS 11
#define VMX_AR_TYPE_BUSY_16_TSS 3
#define VMX_AR_TYPE_LDT 2

#define VMX_AR_UNUSABLE_MASK (1 << 16)
#define VMX_AR_S_MASK (1 << 4)
#define VMX_AR_P_MASK (1 << 7)
#define VMX_AR_L_MASK (1 << 13)
#define VMX_AR_DB_MASK (1 << 14)
#define VMX_AR_G_MASK (1 << 15)
#define VMX_AR_DPL_SHIFT 5
#define VMX_AR_DPL(ar) (((ar) >> VMX_AR_DPL_SHIFT) & 3)

#define VMX_AR_RESERVD_MASK 0xfffe0f00

#define TSS_PRIVATE_MEMSLOT			(KVM_USER_MEM_SLOTS + 0)
#define APIC_ACCESS_PAGE_PRIVATE_MEMSLOT	(KVM_USER_MEM_SLOTS + 1)
#define IDENTITY_PAGETABLE_PRIVATE_MEMSLOT	(KVM_USER_MEM_SLOTS + 2)

#define VMX_NR_VPIDS				(1 << 16)
#define VMX_VPID_EXTENT_INDIVIDUAL_ADDR		0
#define VMX_VPID_EXTENT_SINGLE_CONTEXT		1
#define VMX_VPID_EXTENT_ALL_CONTEXT		2
#define VMX_VPID_EXTENT_SINGLE_NON_GLOBAL	3

#define VMX_EPT_EXTENT_CONTEXT			1
#define VMX_EPT_EXTENT_GLOBAL			2
#define VMX_EPT_EXTENT_SHIFT			24

#define VMX_EPT_EXECUTE_ONLY_BIT		(1ull)
#define VMX_EPT_PAGE_WALK_4_BIT			(1ull << 6)
#define VMX_EPTP_UC_BIT				(1ull << 8)
#define VMX_EPTP_WB_BIT				(1ull << 14)
#define VMX_EPT_2MB_PAGE_BIT			(1ull << 16)
#define VMX_EPT_1GB_PAGE_BIT			(1ull << 17)
#define VMX_EPT_INVEPT_BIT			(1ull << 20)
#define VMX_EPT_AD_BIT				    (1ull << 21)
#define VMX_EPT_EXTENT_CONTEXT_BIT		(1ull << 25)
#define VMX_EPT_EXTENT_GLOBAL_BIT		(1ull << 26)

#define VMX_VPID_INVVPID_BIT                    (1ull << 0) /* (32 - 32) */
#define VMX_VPID_EXTENT_INDIVIDUAL_ADDR_BIT     (1ull << 8) /* (40 - 32) */
#define VMX_VPID_EXTENT_SINGLE_CONTEXT_BIT      (1ull << 9) /* (41 - 32) */
#define VMX_VPID_EXTENT_GLOBAL_CONTEXT_BIT      (1ull << 10) /* (42 - 32) */
#define VMX_VPID_EXTENT_SINGLE_NON_GLOBAL_BIT   (1ull << 11) /* (43 - 32) */

#define VMX_EPT_DEFAULT_GAW			3
#define VMX_EPT_MAX_GAW				0x4
#define VMX_EPT_MT_EPTE_SHIFT			3
#define VMX_EPT_GAW_EPTP_SHIFT			3
#define VMX_EPT_AD_ENABLE_BIT			(1ull << 6)
#define VMX_EPT_DEFAULT_MT			0x6ull
#define VMX_EPT_READABLE_MASK			0x1ull
#define VMX_EPT_WRITABLE_MASK			0x2ull
#define VMX_EPT_EXECUTABLE_MASK			0x4ull
#define VMX_EPT_IPAT_BIT    			(1ull << 6)
#define VMX_EPT_ACCESS_BIT			(1ull << 8)
#define VMX_EPT_DIRTY_BIT			(1ull << 9)
#define VMX_EPT_RWX_MASK                        (VMX_EPT_READABLE_MASK |       \
						 VMX_EPT_WRITABLE_MASK |       \
						 VMX_EPT_EXECUTABLE_MASK)
#define VMX_EPT_MT_MASK				(7ull << VMX_EPT_MT_EPTE_SHIFT)

/* The mask to use to trigger an EPT Misconfiguration in order to track MMIO */
#define VMX_EPT_MISCONFIG_WX_VALUE		(VMX_EPT_WRITABLE_MASK |       \
						 VMX_EPT_EXECUTABLE_MASK)

#define VMX_EPT_IDENTITY_PAGETABLE_ADDR		0xfffbc000ul


#define ASM_VMX_VMCLEAR_RAX       ".byte 0x66, 0x0f, 0xc7, 0x30"
#define ASM_VMX_VMLAUNCH          ".byte 0x0f, 0x01, 0xc2"
#define ASM_VMX_VMRESUME          ".byte 0x0f, 0x01, 0xc3"
#define ASM_VMX_VMPTRLD_RAX       ".byte 0x0f, 0xc7, 0x30"
#define ASM_VMX_VMREAD_RDX_RAX    ".byte 0x0f, 0x78, 0xd0"
#define ASM_VMX_VMWRITE_RAX_RDX   ".byte 0x0f, 0x79, 0xd0"
#define ASM_VMX_VMWRITE_RSP_RDX   ".byte 0x0f, 0x79, 0xd4"
#define ASM_VMX_VMXOFF            ".byte 0x0f, 0x01, 0xc4"
#define ASM_VMX_VMXON_RAX         ".byte 0xf3, 0x0f, 0xc7, 0x30"
#define ASM_VMX_INVEPT		  ".byte 0x66, 0x0f, 0x38, 0x80, 0x08"
#define ASM_VMX_INVVPID		  ".byte 0x66, 0x0f, 0x38, 0x81, 0x08"

struct vmx_msr_entry {
	u32 index;
	u32 reserved;
	u64 value;
};

/*
 * Exit Qualifications for entry failure during or after loading guest state
 */
#define ENTRY_FAIL_DEFAULT		0
#define ENTRY_FAIL_PDPTE		2
#define ENTRY_FAIL_NMI			3
#define ENTRY_FAIL_VMCS_LINK_PTR	4

/*
 * Exit Qualifications for EPT Violations
 */
#define EPT_VIOLATION_ACC_READ_BIT	0
#define EPT_VIOLATION_ACC_WRITE_BIT	1
#define EPT_VIOLATION_ACC_INSTR_BIT	2
#define EPT_VIOLATION_READABLE_BIT	3
#define EPT_VIOLATION_WRITABLE_BIT	4
#define EPT_VIOLATION_EXECUTABLE_BIT	5
#define EPT_VIOLATION_GVA_TRANSLATED_BIT 8
#define EPT_VIOLATION_ACC_READ		(1 << EPT_VIOLATION_ACC_READ_BIT)
#define EPT_VIOLATION_ACC_WRITE		(1 << EPT_VIOLATION_ACC_WRITE_BIT)
#define EPT_VIOLATION_ACC_INSTR		(1 << EPT_VIOLATION_ACC_INSTR_BIT)
#define EPT_VIOLATION_READABLE		(1 << EPT_VIOLATION_READABLE_BIT)
#define EPT_VIOLATION_WRITABLE		(1 << EPT_VIOLATION_WRITABLE_BIT)
#define EPT_VIOLATION_EXECUTABLE	(1 << EPT_VIOLATION_EXECUTABLE_BIT)
#define EPT_VIOLATION_GVA_TRANSLATED	(1 << EPT_VIOLATION_GVA_TRANSLATED_BIT)

/*
 * VM-instruction error numbers
 */
enum vm_instruction_error_number {
	VMXERR_VMCALL_IN_VMX_ROOT_OPERATION = 1,
	VMXERR_VMCLEAR_INVALID_ADDRESS = 2,
	VMXERR_VMCLEAR_VMXON_POINTER = 3,
	VMXERR_VMLAUNCH_NONCLEAR_VMCS = 4,
	VMXERR_VMRESUME_NONLAUNCHED_VMCS = 5,
	VMXERR_VMRESUME_AFTER_VMXOFF = 6,
	VMXERR_ENTRY_INVALID_CONTROL_FIELD = 7,
	VMXERR_ENTRY_INVALID_HOST_STATE_FIELD = 8,
	VMXERR_VMPTRLD_INVALID_ADDRESS = 9,
	VMXERR_VMPTRLD_VMXON_POINTER = 10,
	VMXERR_VMPTRLD_INCORRECT_VMCS_REVISION_ID = 11,
	VMXERR_UNSUPPORTED_VMCS_COMPONENT = 12,
	VMXERR_VMWRITE_READ_ONLY_VMCS_COMPONENT = 13,
	VMXERR_VMXON_IN_VMX_ROOT_OPERATION = 15,
	VMXERR_ENTRY_INVALID_EXECUTIVE_VMCS_POINTER = 16,
	VMXERR_ENTRY_NONLAUNCHED_EXECUTIVE_VMCS = 17,
	VMXERR_ENTRY_EXECUTIVE_VMCS_POINTER_NOT_VMXON_POINTER = 18,
	VMXERR_VMCALL_NONCLEAR_VMCS = 19,
	VMXERR_VMCALL_INVALID_VM_EXIT_CONTROL_FIELDS = 20,
	VMXERR_VMCALL_INCORRECT_MSEG_REVISION_ID = 22,
	VMXERR_VMXOFF_UNDER_DUAL_MONITOR_TREATMENT_OF_SMIS_AND_SMM = 23,
	VMXERR_VMCALL_INVALID_SMM_MONITOR_FEATURES = 24,
	VMXERR_ENTRY_INVALID_VM_EXECUTION_CONTROL_FIELDS_IN_EXECUTIVE_VMCS = 25,
	VMXERR_ENTRY_EVENTS_BLOCKED_BY_MOV_SS = 26,
	VMXERR_INVALID_OPERAND_TO_INVEPT_INVVPID = 28,
};


#define VMX_EXIT_REASONS_FAILED_VMENTRY         0x80000000

#define EXIT_REASON_EXCEPTION_NMI       0
#define EXIT_REASON_EXTERNAL_INTERRUPT  1
#define EXIT_REASON_TRIPLE_FAULT        2

#define EXIT_REASON_PENDING_INTERRUPT   7
#define EXIT_REASON_NMI_WINDOW          8
#define EXIT_REASON_TASK_SWITCH         9
#define EXIT_REASON_CPUID               10
#define EXIT_REASON_HLT                 12
#define EXIT_REASON_INVD                13
#define EXIT_REASON_INVLPG              14
#define EXIT_REASON_RDPMC               15
#define EXIT_REASON_RDTSC               16
#define EXIT_REASON_VMCALL              18
#define EXIT_REASON_VMCLEAR             19
#define EXIT_REASON_VMLAUNCH            20
#define EXIT_REASON_VMPTRLD             21
#define EXIT_REASON_VMPTRST             22
#define EXIT_REASON_VMREAD              23
#define EXIT_REASON_VMRESUME            24
#define EXIT_REASON_VMWRITE             25
#define EXIT_REASON_VMOFF               26
#define EXIT_REASON_VMON                27
#define EXIT_REASON_CR_ACCESS           28
#define EXIT_REASON_DR_ACCESS           29
#define EXIT_REASON_IO_INSTRUCTION      30
#define EXIT_REASON_MSR_READ            31
#define EXIT_REASON_MSR_WRITE           32
#define EXIT_REASON_INVALID_STATE       33
#define EXIT_REASON_MSR_LOAD_FAIL       34
#define EXIT_REASON_MWAIT_INSTRUCTION   36
#define EXIT_REASON_MONITOR_TRAP_FLAG   37
#define EXIT_REASON_MONITOR_INSTRUCTION 39
#define EXIT_REASON_PAUSE_INSTRUCTION   40
#define EXIT_REASON_MCE_DURING_VMENTRY  41
#define EXIT_REASON_TPR_BELOW_THRESHOLD 43
#define EXIT_REASON_APIC_ACCESS         44
#define EXIT_REASON_EOI_INDUCED         45
#define EXIT_REASON_GDTR_IDTR           46
#define EXIT_REASON_LDTR_TR             47
#define EXIT_REASON_EPT_VIOLATION       48
#define EXIT_REASON_EPT_MISCONFIG       49
#define EXIT_REASON_INVEPT              50
#define EXIT_REASON_RDTSCP              51
#define EXIT_REASON_PREEMPTION_TIMER    52
#define EXIT_REASON_INVVPID             53
#define EXIT_REASON_WBINVD              54
#define EXIT_REASON_XSETBV              55
#define EXIT_REASON_APIC_WRITE          56
#define EXIT_REASON_RDRAND              57
#define EXIT_REASON_INVPCID             58
#define EXIT_REASON_VMFUNC              59
#define EXIT_REASON_ENCLS               60
#define EXIT_REASON_RDSEED              61
#define EXIT_REASON_PML_FULL            62
#define EXIT_REASON_XSAVES              63
#define EXIT_REASON_XRSTORS             64

#define VMX_EXIT_REASONS \
	{ EXIT_REASON_EXCEPTION_NMI,         "EXCEPTION_NMI" }, \
	{ EXIT_REASON_EXTERNAL_INTERRUPT,    "EXTERNAL_INTERRUPT" }, \
	{ EXIT_REASON_TRIPLE_FAULT,          "TRIPLE_FAULT" }, \
	{ EXIT_REASON_PENDING_INTERRUPT,     "PENDING_INTERRUPT" }, \
	{ EXIT_REASON_NMI_WINDOW,            "NMI_WINDOW" }, \
	{ EXIT_REASON_TASK_SWITCH,           "TASK_SWITCH" }, \
	{ EXIT_REASON_CPUID,                 "CPUID" }, \
	{ EXIT_REASON_HLT,                   "HLT" }, \
	{ EXIT_REASON_INVD,                  "INVD" }, \
	{ EXIT_REASON_INVLPG,                "INVLPG" }, \
	{ EXIT_REASON_RDPMC,                 "RDPMC" }, \
	{ EXIT_REASON_RDTSC,                 "RDTSC" }, \
	{ EXIT_REASON_VMCALL,                "VMCALL" }, \
	{ EXIT_REASON_VMCLEAR,               "VMCLEAR" }, \
	{ EXIT_REASON_VMLAUNCH,              "VMLAUNCH" }, \
	{ EXIT_REASON_VMPTRLD,               "VMPTRLD" }, \
	{ EXIT_REASON_VMPTRST,               "VMPTRST" }, \
	{ EXIT_REASON_VMREAD,                "VMREAD" }, \
	{ EXIT_REASON_VMRESUME,              "VMRESUME" }, \
	{ EXIT_REASON_VMWRITE,               "VMWRITE" }, \
	{ EXIT_REASON_VMOFF,                 "VMOFF" }, \
	{ EXIT_REASON_VMON,                  "VMON" }, \
	{ EXIT_REASON_CR_ACCESS,             "CR_ACCESS" }, \
	{ EXIT_REASON_DR_ACCESS,             "DR_ACCESS" }, \
	{ EXIT_REASON_IO_INSTRUCTION,        "IO_INSTRUCTION" }, \
	{ EXIT_REASON_MSR_READ,              "MSR_READ" }, \
	{ EXIT_REASON_MSR_WRITE,             "MSR_WRITE" }, \
	{ EXIT_REASON_INVALID_STATE,         "INVALID_STATE" }, \
	{ EXIT_REASON_MSR_LOAD_FAIL,         "MSR_LOAD_FAIL" }, \
	{ EXIT_REASON_MWAIT_INSTRUCTION,     "MWAIT_INSTRUCTION" }, \
	{ EXIT_REASON_MONITOR_TRAP_FLAG,     "MONITOR_TRAP_FLAG" }, \
	{ EXIT_REASON_MONITOR_INSTRUCTION,   "MONITOR_INSTRUCTION" }, \
	{ EXIT_REASON_PAUSE_INSTRUCTION,     "PAUSE_INSTRUCTION" }, \
	{ EXIT_REASON_MCE_DURING_VMENTRY,    "MCE_DURING_VMENTRY" }, \
	{ EXIT_REASON_TPR_BELOW_THRESHOLD,   "TPR_BELOW_THRESHOLD" }, \
	{ EXIT_REASON_APIC_ACCESS,           "APIC_ACCESS" }, \
	{ EXIT_REASON_EOI_INDUCED,           "EOI_INDUCED" }, \
	{ EXIT_REASON_GDTR_IDTR,             "GDTR_IDTR" }, \
	{ EXIT_REASON_LDTR_TR,               "LDTR_TR" }, \
	{ EXIT_REASON_EPT_VIOLATION,         "EPT_VIOLATION" }, \
	{ EXIT_REASON_EPT_MISCONFIG,         "EPT_MISCONFIG" }, \
	{ EXIT_REASON_INVEPT,                "INVEPT" }, \
	{ EXIT_REASON_RDTSCP,                "RDTSCP" }, \
	{ EXIT_REASON_PREEMPTION_TIMER,      "PREEMPTION_TIMER" }, \
	{ EXIT_REASON_INVVPID,               "INVVPID" }, \
	{ EXIT_REASON_WBINVD,                "WBINVD" }, \
	{ EXIT_REASON_XSETBV,                "XSETBV" }, \
	{ EXIT_REASON_APIC_WRITE,            "APIC_WRITE" }, \
	{ EXIT_REASON_RDRAND,                "RDRAND" }, \
	{ EXIT_REASON_INVPCID,               "INVPCID" }, \
	{ EXIT_REASON_VMFUNC,                "VMFUNC" }, \
	{ EXIT_REASON_ENCLS,                 "ENCLS" }, \
	{ EXIT_REASON_RDSEED,                "RDSEED" }, \
	{ EXIT_REASON_PML_FULL,              "PML_FULL" }, \
	{ EXIT_REASON_XSAVES,                "XSAVES" }, \
	{ EXIT_REASON_XRSTORS,               "XRSTORS" }

#define VMX_ABORT_SAVE_GUEST_MSR_FAIL        1
#define VMX_ABORT_LOAD_HOST_PDPTE_FAIL       2
#define VMX_ABORT_LOAD_HOST_MSR_FAIL         4


/* EPTP Attributes. */
#define EPTP_CACHE_WB 0x6
#define EPTP_UNCACHEABLE 0x0
#define EPTP_PAGE_WALK_LEN(x) (((x)&0x7)<<3)
#define EPTP_ENABLE_A_D_FLAGS (1 << 6)
#define EPT_IGNORE_PAT (1 << 6)

/* EPT Attributes. */
#define EPT_PML4E_READ (1 << 0)
#define EPT_PML4E_WRITE (1 << 1)
#define EPT_PML4E_EXECUTE (1 << 2)
#define EPT_PML4E_ACCESS_FLAG (1 << 8)

#define EPT_PDPTE_READ (1 << 0)
#define EPT_PDPTE_WRITE (1 << 1)
#define EPT_PDPTE_EXECUTE (1 << 2)
#define EPT_PDPTE_UNCACHEABLE (0 << 3)
#define EPT_PDPTE_CACHE_WC (1 << 3)
#define EPT_PDPTE_CACHE_WT (4 << 3)
#define EPT_PDPTE_CACHE_WP (5 << 3)
#define EPT_PDPTE_CACHE_WB (6 << 3)
#define EPT_PDPTE_CACHE_IGNORE_PAT (1 << 6)
#define EPT_PDPTE_1GB_PAGE (1 << 7)
#define EPT_PDPTE_ACCESS_FLAG (1 << 8)
#define EPT_PDPTE_DIRTY_FLAG (1 << 9)

#define EPT_PDE_READ (1 << 0)
#define EPT_PDE_WRITE (1 << 1)
#define EPT_PDE_EXECUTE (1 << 2)
#define EPT_PDE_UNCACHEABLE (0 << 3)
#define EPT_PDE_CACHE_WC (1 << 3)
#define EPT_PDE_CACHE_WT (4 << 3)
#define EPT_PDE_CACHE_WP (5 << 3)
#define EPT_PDE_CACHE_WB (6 << 3)
#define EPT_PDE_CACHE_IGNORE_PAT (1 << 6)
#define EPT_PDE_2MB_PAGE (1 << 7)
#define EPT_PDE_ACCESS_FLAG (1 << 8)
#define EPT_PDE_DIRTY_FLAG (1 << 9)

#define EPT_PTE_READ (1 << 0)
#define EPT_PTE_WRITE (1 << 1)
#define EPT_PTE_EXECUTE (1 << 2)
#define EPT_PTE_UNCACHEABLE (0 << 3)
#define EPT_PTE_CACHE_WC (1 << 3)
#define EPT_PTE_CACHE_WT (4 << 3)
#define EPT_PTE_CACHE_WP (5 << 3)
#define EPT_PTE_CACHE_WB (6 << 3)
#define EPT_PTE_CACHE_IGNORE_PAT (1 << 6)
#define EPT_PTE_ACCESS_FLAG (1 << 8)
#define EPT_PTE_DIRTY_FLAG (1 << 9)






/*
 * struct vmcs12 describes the state that our guest hypervisor (L1) keeps for a
 * single nested guest (L2), hence the name vmcs12. Any VMX implementation has
 * a VMCS structure, and vmcs12 is our emulated VMX's VMCS. This structure is
 * stored in guest memory specified by VMPTRLD, but is opaque to the guest,
 * which must access it using VMREAD/VMWRITE/VMCLEAR instructions.
 * More than one of these structures may exist, if L1 runs multiple L2 guests.
 * nested_vmx_run() will use the data here to build a vmcs02: a VMCS for the
 * underlying hardware which will be used to run L2.
 * This structure is packed to ensure that its layout is identical across
 * machines (necessary for live migration).
 * If there are changes in this struct, VMCS12_REVISION must be changed.
 */
typedef u64 natural_width;
typedef struct vmcs12 {
	/* According to the Intel spec, a VMCS region must start with the
	 * following two fields. Then follow implementation-specific data.
	 */
	u32 revision_id;
	u32 abort;

	u32 launch_state; /* set to 0 by VMCLEAR, to 1 by VMLAUNCH */
	u32 padding[7]; /* room for future expansion */

	u64 io_bitmap_a;
	u64 io_bitmap_b;
	u64 msr_bitmap;
	u64 vm_exit_msr_store_addr;
	u64 vm_exit_msr_load_addr;
	u64 vm_entry_msr_load_addr;
	u64 tsc_offset;
	u64 virtual_apic_page_addr;
	u64 apic_access_addr;
	u64 posted_intr_desc_addr;
	u64 ept_pointer;
	u64 eoi_exit_bitmap0;
	u64 eoi_exit_bitmap1;
	u64 eoi_exit_bitmap2;
	u64 eoi_exit_bitmap3;
	u64 xss_exit_bitmap;
	u64 guest_physical_address;
	u64 vmcs_link_pointer;
	u64 pml_address;
	u64 guest_ia32_debugctl;
	u64 guest_ia32_pat;
	u64 guest_ia32_efer;
	u64 guest_ia32_perf_global_ctrl;
	u64 guest_pdptr0;
	u64 guest_pdptr1;
	u64 guest_pdptr2;
	u64 guest_pdptr3;
	u64 guest_bndcfgs;
	u64 host_ia32_pat;
	u64 host_ia32_efer;
	u64 host_ia32_perf_global_ctrl;
	u64 padding64[8]; /* room for future expansion */
	/*
	 * To allow migration of L1 (complete with its L2 guests) between
	 * machines of different natural widths (32 or 64 bit), we cannot have
	 * unsigned long fields with no explict size. We use u64 (aliased
	 * natural_width) instead. Luckily, x86 is little-endian.
	 */
	natural_width cr0_guest_host_mask;
	natural_width cr4_guest_host_mask;
	natural_width cr0_read_shadow;
	natural_width cr4_read_shadow;
	natural_width cr3_target_value0;
	natural_width cr3_target_value1;
	natural_width cr3_target_value2;
	natural_width cr3_target_value3;
	natural_width exit_qualification;
	natural_width guest_linear_address;
	natural_width guest_cr0;
	natural_width guest_cr3;
	natural_width guest_cr4;
	natural_width guest_es_base;
	natural_width guest_cs_base;
	natural_width guest_ss_base;
	natural_width guest_ds_base;
	natural_width guest_fs_base;
	natural_width guest_gs_base;
	natural_width guest_ldtr_base;
	natural_width guest_tr_base;
	natural_width guest_gdtr_base;
	natural_width guest_idtr_base;
	natural_width guest_dr7;
	natural_width guest_rsp;
	natural_width guest_rip;
	natural_width guest_rflags;
	natural_width guest_pending_dbg_exceptions;
	natural_width guest_sysenter_esp;
	natural_width guest_sysenter_eip;
	natural_width host_cr0;
	natural_width host_cr3;
	natural_width host_cr4;
	natural_width host_fs_base;
	natural_width host_gs_base;
	natural_width host_tr_base;
	natural_width host_gdtr_base;
	natural_width host_idtr_base;
	natural_width host_ia32_sysenter_esp;
	natural_width host_ia32_sysenter_eip;
	natural_width host_rsp;
	natural_width host_rip;
	natural_width paddingl[8]; /* room for future expansion */
	u32 pin_based_vm_exec_control;
	u32 cpu_based_vm_exec_control;
	u32 exception_bitmap;
	u32 page_fault_error_code_mask;
	u32 page_fault_error_code_match;
	u32 cr3_target_count;
	u32 vm_exit_controls;
	u32 vm_exit_msr_store_count;
	u32 vm_exit_msr_load_count;
	u32 vm_entry_controls;
	u32 vm_entry_msr_load_count;
	u32 vm_entry_intr_info_field;
	u32 vm_entry_exception_error_code;
	u32 vm_entry_instruction_len;
	u32 tpr_threshold;
	u32 secondary_vm_exec_control;
	u32 vm_instruction_error;
	u32 vm_exit_reason;
	u32 vm_exit_intr_info;
	u32 vm_exit_intr_error_code;
	u32 idt_vectoring_info_field;
	u32 idt_vectoring_error_code;
	u32 vm_exit_instruction_len;
	u32 vmx_instruction_info;
	u32 guest_es_limit;
	u32 guest_cs_limit;
	u32 guest_ss_limit;
	u32 guest_ds_limit;
	u32 guest_fs_limit;
	u32 guest_gs_limit;
	u32 guest_ldtr_limit;
	u32 guest_tr_limit;
	u32 guest_gdtr_limit;
	u32 guest_idtr_limit;
	u32 guest_es_ar_bytes;
	u32 guest_cs_ar_bytes;
	u32 guest_ss_ar_bytes;
	u32 guest_ds_ar_bytes;
	u32 guest_fs_ar_bytes;
	u32 guest_gs_ar_bytes;
	u32 guest_ldtr_ar_bytes;
	u32 guest_tr_ar_bytes;
	u32 guest_interruptibility_info;
	u32 guest_activity_state;
	u32 guest_sysenter_cs;
	u32 host_ia32_sysenter_cs;
	u32 vmx_preemption_timer_value;
	u32 padding32[7]; /* room for future expansion */
	u16 virtual_processor_id;
	u16 posted_intr_nv;
	u16 guest_es_selector;
	u16 guest_cs_selector;
	u16 guest_ss_selector;
	u16 guest_ds_selector;
	u16 guest_fs_selector;
	u16 guest_gs_selector;
	u16 guest_ldtr_selector;
	u16 guest_tr_selector;
	u16 guest_intr_status;
	u16 guest_pml_index;
	u16 host_es_selector;
	u16 host_cs_selector;
	u16 host_ss_selector;
	u16 host_ds_selector;
	u16 host_fs_selector;
	u16 host_gs_selector;
	u16 host_tr_selector;
}VMCS_SHADOW;


typedef struct _POSTED_INTERRUPT_DESCRIPTOR
{
	UINT8 Vector[32];
	UINT8 OutStanding:1;
	UINT8 Reserved:7;
	UINT8 Reserved0[31];
}POSTED_INT_DESC;


typedef struct vmcs_host
{
	void *io_bitmap_a;
	void *io_bitmap_b;
	void *msr_bitmap;
	void *vm_exit_msr_store_addr;
	void *vm_exit_msr_load_addr;
	void *vm_entry_msr_load_addr;
	void *virtual_apic_page_addr;
	POSTED_INT_DESC *posted_intr_desc_addr;
	void *ept_pointer;
	void *xss_exit_bitmap;
	void *pml_address;
}VMCS_HOST_MAP;

typedef struct _REG_STATE
{
	UINT64 R15;
	UINT64 R14;
	UINT64 R13;
	UINT64 R12;
	UINT64 R11;
	UINT64 R10;
	UINT64 R9;
	UINT64 R8;
	UINT64 RDI;
	UINT64 RSI;
	UINT64 RBP;
	UINT64 RSP;
	UINT64 RDX;
	UINT64 RCX;
	UINT64 RBX;
	UINT64 RAX;
}REG_STATE;


typedef struct _X86_VMX_VCPU
{
	UINT64 VmxOnRegionPhysAddr;
	VOID *VmxOnRegionVirtAddr;
	UINT64 VmxRegionPhysAddr;
	VOID *VmxRegionVirtAddr;

	VMCS_SHADOW Vmcs;
	VMCS_HOST_MAP VmcsVirt;

	REG_STATE HostRegs;
	REG_STATE GuestRegs;
}X86_VMX_VCPU;




VOID VmxOn(UINT64 VmxOnRegionPhysAddr);
VOID VmxOff(VOID);
RETURN_STATUS VmLaunch(REG_STATE *HostReg, REG_STATE *GuestReg);

RETURN_STATUS VmResume(REG_STATE *HostReg, REG_STATE *GuestReg);

//Warning:This function may not be called directly.This pointer should be written to vmcs.host_rip.
VOID VmExit(VOID);

VOID VmPtrLoad(UINT64 VmcsPhysAddr);
VOID VmPtrStore(UINT64 * VmcsPhysAddr);
VOID VmClear(UINT64 VmcsPhysAddr);
UINT64 VmRead(UINT64 VmcsID);
VOID VmWrite(UINT64 VmcsID, UINT64 Value);
VOID VmCall(VOID);
VOID VmFunc(UINT32 VmFun);
VOID InvalidateEpt(UINT64 Type, void * Ptr);
VOID InvalidateVpid(UINT64 Type, void * Ptr);

RETURN_STATUS VmxSupport();
RETURN_STATUS VmxStatus();
RETURN_STATUS VmxEnable();
RETURN_STATUS VmxOnPrepare(VOID * VmxOnRegionPtr);
RETURN_STATUS VmOn(UINT64 VmxOnRegionPhys);
RETURN_STATUS VmOff(VOID);

X86_VMX_VCPU *VmxNew();
RETURN_STATUS VmxInit(X86_VMX_VCPU *Vcpu);

VOID VmxCpuBistState(X86_VMX_VCPU *Vcpu);
VOID VmxCopyHostState(X86_VMX_VCPU *Vcpu);
VOID VmxSetVmcs(struct vmcs12 *vmcs_ptr);
VOID GetVmxState(struct vmcs12 *vmcs_ptr);

RETURN_STATUS VcpuRun(VCPU *Vcpu);
RETURN_STATUS VmExitHandler(VCPU *Vcpu);





#endif
