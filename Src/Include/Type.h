//=====================================================================
//Type.h
//
//Oscar 2015.4
//=====================================================================
#ifndef _TYPE_H
#define _TYPE_H

typedef unsigned long long	UINT64;
typedef long				INT64;
typedef unsigned int		UINT32; 
typedef int 				INT32;  
typedef unsigned short		UINT16;
typedef short int			INT16;
typedef short int			CHAR16;
typedef unsigned char		UINT8;
typedef char				INT8;
typedef char				CHAR8;
typedef void				VOID;
typedef UINT8 				BOOLEAN;
typedef unsigned long       UINTN;

typedef long                RETURN_STATUS;

/* For linux kernel porting */
typedef unsigned long long	u64;
typedef long				s64;
typedef unsigned int		u32;
typedef int 				s32;
typedef unsigned short		u16;
typedef short int			s16;
typedef unsigned char		u8;
typedef char				s8;
typedef unsigned char		bool;

typedef unsigned long long	uint64_t;
typedef long				int64_t;
typedef unsigned int		uint32_t;
typedef int 				int32_t;
typedef unsigned short		uint16_t;
typedef short int			int16_t;
typedef unsigned char		uint8_t;
typedef char				int8_t;
#define true				1
#define false				0

#define container_of(x,y,z) ContainerOf(x,y,z)

typedef struct {
	UINT32  Data1;
	UINT16  Data2;
	UINT16  Data3;
	UINT8   Data4[8];
} GUID;


#define CONST     const
#define STATIC    static

#define IN
#define OUT
#define OPTIONAL

#define TRUE  ((UINT8)(1==1))
#define FALSE ((UINT8)(0==1))
#define NULL  ((VOID *) 0)

#define MAX_INT8    ((INT8)0x7F)
#define MAX_UINT8   ((UINT8)0xFF)
#define MAX_INT16   ((INT16)0x7FFF)
#define MAX_UINT16  ((UINT16)0xFFFF)
#define MAX_INT32   ((INT32)0x7FFFFFFF)
#define MAX_UINT32  ((UINT32)0xFFFFFFFF)
#define MAX_INT64   ((INT64)0x7FFFFFFFFFFFFFFFULL)
#define MAX_UINT64  ((UINT64)0xFFFFFFFFFFFFFFFFULL)

#define  BIT0     0x00000001
#define  BIT1     0x00000002
#define  BIT2     0x00000004
#define  BIT3     0x00000008
#define  BIT4     0x00000010
#define  BIT5     0x00000020
#define  BIT6     0x00000040
#define  BIT7     0x00000080
#define  BIT8     0x00000100
#define  BIT9     0x00000200
#define  BIT10    0x00000400
#define  BIT11    0x00000800
#define  BIT12    0x00001000
#define  BIT13    0x00002000
#define  BIT14    0x00004000
#define  BIT15    0x00008000
#define  BIT16    0x00010000
#define  BIT17    0x00020000
#define  BIT18    0x00040000
#define  BIT19    0x00080000
#define  BIT20    0x00100000
#define  BIT21    0x00200000
#define  BIT22    0x00400000
#define  BIT23    0x00800000
#define  BIT24    0x01000000
#define  BIT25    0x02000000
#define  BIT26    0x04000000
#define  BIT27    0x08000000
#define  BIT28    0x10000000
#define  BIT29    0x20000000
#define  BIT30    0x40000000
#define  BIT31    0x80000000
#define  BIT32    0x0000000100000000ULL
#define  BIT33    0x0000000200000000ULL
#define  BIT34    0x0000000400000000ULL
#define  BIT35    0x0000000800000000ULL
#define  BIT36    0x0000001000000000ULL
#define  BIT37    0x0000002000000000ULL
#define  BIT38    0x0000004000000000ULL
#define  BIT39    0x0000008000000000ULL
#define  BIT40    0x0000010000000000ULL
#define  BIT41    0x0000020000000000ULL
#define  BIT42    0x0000040000000000ULL
#define  BIT43    0x0000080000000000ULL
#define  BIT44    0x0000100000000000ULL
#define  BIT45    0x0000200000000000ULL
#define  BIT46    0x0000400000000000ULL
#define  BIT47    0x0000800000000000ULL
#define  BIT48    0x0001000000000000ULL
#define  BIT49    0x0002000000000000ULL
#define  BIT50    0x0004000000000000ULL
#define  BIT51    0x0008000000000000ULL
#define  BIT52    0x0010000000000000ULL
#define  BIT53    0x0020000000000000ULL
#define  BIT54    0x0040000000000000ULL
#define  BIT55    0x0080000000000000ULL
#define  BIT56    0x0100000000000000ULL
#define  BIT57    0x0200000000000000ULL
#define  BIT58    0x0400000000000000ULL
#define  BIT59    0x0800000000000000ULL
#define  BIT60    0x1000000000000000ULL
#define  BIT61    0x2000000000000000ULL
#define  BIT62    0x4000000000000000ULL
#define  BIT63    0x8000000000000000ULL

#define  SIZE_1KB    0x00000400
#define  SIZE_2KB    0x00000800
#define  SIZE_4KB    0x00001000
#define  SIZE_8KB    0x00002000
#define  SIZE_16KB   0x00004000
#define  SIZE_32KB   0x00008000
#define  SIZE_64KB   0x00010000
#define  SIZE_128KB  0x00020000
#define  SIZE_256KB  0x00040000
#define  SIZE_512KB  0x00080000
#define  SIZE_1MB    0x00100000
#define  SIZE_2MB    0x00200000
#define  SIZE_4MB    0x00400000
#define  SIZE_8MB    0x00800000
#define  SIZE_16MB   0x01000000
#define  SIZE_32MB   0x02000000
#define  SIZE_64MB   0x04000000
#define  SIZE_128MB  0x08000000
#define  SIZE_256MB  0x10000000
#define  SIZE_512MB  0x20000000
#define  SIZE_1GB    0x40000000
#define  SIZE_2GB    0x80000000
#define  SIZE_4GB    0x0000000100000000ULL
#define  SIZE_8GB    0x0000000200000000ULL
#define  SIZE_16GB   0x0000000400000000ULL
#define  SIZE_32GB   0x0000000800000000ULL
#define  SIZE_64GB   0x0000001000000000ULL
#define  SIZE_128GB  0x0000002000000000ULL
#define  SIZE_256GB  0x0000004000000000ULL
#define  SIZE_512GB  0x0000008000000000ULL
#define  SIZE_1TB    0x0000010000000000ULL
#define  SIZE_2TB    0x0000020000000000ULL
#define  SIZE_4TB    0x0000040000000000ULL
#define  SIZE_8TB    0x0000080000000000ULL
#define  SIZE_16TB   0x0000100000000000ULL
#define  SIZE_32TB   0x0000200000000000ULL
#define  SIZE_64TB   0x0000400000000000ULL
#define  SIZE_128TB  0x0000800000000000ULL
#define  SIZE_256TB  0x0001000000000000ULL
#define  SIZE_512TB  0x0002000000000000ULL
#define  SIZE_1PB    0x0004000000000000ULL
#define  SIZE_2PB    0x0008000000000000ULL
#define  SIZE_4PB    0x0010000000000000ULL
#define  SIZE_8PB    0x0020000000000000ULL
#define  SIZE_16PB   0x0040000000000000ULL
#define  SIZE_32PB   0x0080000000000000ULL
#define  SIZE_64PB   0x0100000000000000ULL
#define  SIZE_128PB  0x0200000000000000ULL
#define  SIZE_256PB  0x0400000000000000ULL
#define  SIZE_512PB  0x0800000000000000ULL
#define  SIZE_1EB    0x1000000000000000ULL
#define  SIZE_2EB    0x2000000000000000ULL
#define  SIZE_4EB    0x4000000000000000ULL
#define  SIZE_8EB    0x8000000000000000ULL

#define  BASE_1KB    0x00000400
#define  BASE_2KB    0x00000800
#define  BASE_4KB    0x00001000
#define  BASE_8KB    0x00002000
#define  BASE_16KB   0x00004000
#define  BASE_32KB   0x00008000
#define  BASE_64KB   0x00010000
#define  BASE_128KB  0x00020000
#define  BASE_256KB  0x00040000
#define  BASE_512KB  0x00080000
#define  BASE_1MB    0x00100000
#define  BASE_2MB    0x00200000
#define  BASE_4MB    0x00400000
#define  BASE_8MB    0x00800000
#define  BASE_16MB   0x01000000
#define  BASE_32MB   0x02000000
#define  BASE_64MB   0x04000000
#define  BASE_128MB  0x08000000
#define  BASE_256MB  0x10000000
#define  BASE_512MB  0x20000000
#define  BASE_1GB    0x40000000
#define  BASE_2GB    0x80000000
#define  BASE_4GB    0x0000000100000000ULL
#define  BASE_8GB    0x0000000200000000ULL
#define  BASE_16GB   0x0000000400000000ULL
#define  BASE_32GB   0x0000000800000000ULL
#define  BASE_64GB   0x0000001000000000ULL
#define  BASE_128GB  0x0000002000000000ULL
#define  BASE_256GB  0x0000004000000000ULL
#define  BASE_512GB  0x0000008000000000ULL
#define  BASE_1TB    0x0000010000000000ULL
#define  BASE_2TB    0x0000020000000000ULL
#define  BASE_4TB    0x0000040000000000ULL
#define  BASE_8TB    0x0000080000000000ULL
#define  BASE_16TB   0x0000100000000000ULL
#define  BASE_32TB   0x0000200000000000ULL
#define  BASE_64TB   0x0000400000000000ULL
#define  BASE_128TB  0x0000800000000000ULL
#define  BASE_256TB  0x0001000000000000ULL
#define  BASE_512TB  0x0002000000000000ULL
#define  BASE_1PB    0x0004000000000000ULL
#define  BASE_2PB    0x0008000000000000ULL
#define  BASE_4PB    0x0010000000000000ULL
#define  BASE_8PB    0x0020000000000000ULL
#define  BASE_16PB   0x0040000000000000ULL
#define  BASE_32PB   0x0080000000000000ULL
#define  BASE_64PB   0x0100000000000000ULL
#define  BASE_128PB  0x0200000000000000ULL
#define  BASE_256PB  0x0400000000000000ULL
#define  BASE_512PB  0x0800000000000000ULL
#define  BASE_1EB    0x1000000000000000ULL
#define  BASE_2EB    0x2000000000000000ULL
#define  BASE_4EB    0x4000000000000000ULL
#define  BASE_8EB    0x8000000000000000ULL

#define ENCODE_ERROR(a)              ((1ULL<<(sizeof(long)*8-1)) | (a))
#define RETURN_ERROR(a)              ((a) < 0)

#define RETURN_SUCCESS               0
#define RETURN_LOAD_ERROR            ENCODE_ERROR (1)
#define RETURN_INVALID_PARAMETER     ENCODE_ERROR (2)
#define RETURN_UNSUPPORTED           ENCODE_ERROR (3)
#define RETURN_BAD_BUFFER_SIZE       ENCODE_ERROR (4)
#define RETURN_BUFFER_TOO_SMALL      ENCODE_ERROR (5)
#define RETURN_NOT_READY             ENCODE_ERROR (6)
#define RETURN_DEVICE_ERROR          ENCODE_ERROR (7)
#define RETURN_WRITE_PROTECTED       ENCODE_ERROR (8)
#define RETURN_OUT_OF_RESOURCES      ENCODE_ERROR (9)
#define RETURN_VOLUME_CORRUPTED      ENCODE_ERROR (10)
#define RETURN_VOLUME_FULL           ENCODE_ERROR (11)
#define RETURN_NO_MEDIA              ENCODE_ERROR (12)
#define RETURN_MEDIA_CHANGED         ENCODE_ERROR (13)
#define RETURN_NOT_FOUND             ENCODE_ERROR (14)
#define RETURN_ACCESS_DENIED         ENCODE_ERROR (15)
#define RETURN_NO_RESPONSE           ENCODE_ERROR (16)
#define RETURN_NO_MAPPING            ENCODE_ERROR (17)
#define RETURN_TIMEOUT               ENCODE_ERROR (18)
#define RETURN_NOT_STARTED           ENCODE_ERROR (19)
#define RETURN_ALREADY_STARTED       ENCODE_ERROR (20)
#define RETURN_ABORTED               ENCODE_ERROR (21)
#define RETURN_ICMP_ERROR            ENCODE_ERROR (22)
#define RETURN_TFTP_ERROR            ENCODE_ERROR (23)
#define RETURN_PROTOCOL_ERROR        ENCODE_ERROR (24)
#define RETURN_INCOMPATIBLE_VERSION  ENCODE_ERROR (25)
#define RETURN_SECURITY_VIOLATION    ENCODE_ERROR (26)
#define RETURN_CRC_ERROR             ENCODE_ERROR (27)
#define RETURN_END_OF_MEDIA          ENCODE_ERROR (28)
#define RETURN_END_OF_FILE           ENCODE_ERROR (31)

/*linux kernel error code.*/
#define EPERM				1	/* Operation not permitted */
#define ENOENT				2    /* No such file or directory */
#define ESRCH				3     /* No such process */
#define EINTR				4    /* Interrupted system call */
#define EIO					5    /* I/O error */
#define ENXIO				6    /* No such device or address */
#define E2BIG				7    /* Argument list too long */
#define ENOEXEC				8    /* Exec format error */
#define EBADF				9    /* Bad file number */
#define ECHILD				10   /* No child processes */
#define EAGAIN				11   /* Try again */
#define ENOMEM				12   /* Out of memory */
#define EACCES				13   /* Permission denied */
#define EFAULT				14   /* Bad address */
#define ENOTBLK				15   /* Block device required */
#define EBUSY				16   /* Device or resource busy */
#define EEXIST				17   /* File exists */
#define EXDEV				18   /* Cross-device link */
#define ENODEV				19   /* No such device */
#define ENOTDIR				20   /* Not a directory */
#define EISDIR				21   /* Is a directory */
#define EINVAL				22   /* Invalid argument */
#define ENFILE				23   /* File table overflow */
#define EMFILE				24   /* Too many open files */
#define ENOTTY				25   /* Not a typewriter */
#define ETXTBSY				26   /* Text file busy */
#define EFBIG				27   /* File too large */
#define ENOSPC				28   /* No space left on device */
#define ESPIPE				29   /* Illegal seek */
#define EROFS				30   /* Read-only file system */
#define EMLINK				31   /* Too many links */
#define EPIPE				32   /* Broken pipe */
#define EDOM				33   /* Math argument out of domain of func */
#define ERANGE				34   /* Math result not representable */

#define EDEADLK				35   /* Resource deadlock would occur */
#define ENAMETOOLONG		36   /* File name too long */
#define ENOLCK				37   /* No record locks available */
#define ENOSYS				38   /* Function not implemented */
#define ENOTEMPTY			39   /* Directory not empty */
#define ELOOP				40   /* Too many symbolic links encountered */
#define EWOULDBLOCK			EAGAIN   /* Operation would block */
#define ENOMSG				42   /* No message of desired type */
#define EIDRM				43   /* Identifier removed */
#define ECHRNG				44   /* Channel number out of range */
#define EL2NSYNC			45   /* Level 2 not synchronized */
#define EL3HLT				46   /* Level 3 halted */
#define EL3RST				47   /* Level 3 reset */
#define ELNRNG				48   /* Link number out of range */
#define EUNATCH				49   /* Protocol driver not attached */
#define ENOCSI				50   /* No CSI structure available */
#define EL2HLT				51   /* Level 2 halted */
#define EBADE				52   /* Invalid exchange */
#define EBADR				53   /* Invalid request descriptor */
#define EXFULL				54   /* Exchange full */
#define ENOANO				55   /* No anode */
#define EBADRQC				56   /* Invalid request code */
#define EBADSLT				57   /* Invalid slot */

#define ETIMEDOUT			58

#endif