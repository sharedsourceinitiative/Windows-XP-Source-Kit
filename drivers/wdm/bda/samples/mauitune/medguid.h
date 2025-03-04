#ifndef _MEDGUID_H_
#define _MEDGUID_H_

//
// Definition of Medium GUIDs used in SAA7146 WDM mini-driver and related driver
                               
#define GUID_DMSD_IN    0x328c7240, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34

#define GUID_DMSD_OUT   0x328c7241, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34

#define GUID_TV_VIDEO   0x328c7242, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34
#define GUID_TV_AUDIO   0x328c7243, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34

#define GUID_7146IN     0x328c7241, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34

#define GUID_6750_OUT   0x328c7245, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34
#define GUID_7146_DEBI  0x328c7245, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34

// output to BRS connection (video encoder)
#define GUID_BRS_OUT    0x328c7246, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34

// output to DEBI Output connection (eg MPEG Decoder)
// {BAF13000-33A7-11d3-A4D9-0060080BA634}
#define GUID_DEBI_OUT   0xbaf13000, 0x33a7, 0x11d3, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34
                        

// from jaybo
// {F3954424-34F6-11D1-821D-0000F8300212}
#define GUID_VSB_OUT    0xf3954424, 0x34f6, 0x11d1, 0x82, 0x1d, 0x0, 0x0, 0xf8, 0x30, 0x2, 0x12
#define GUID_7146XPIN   0xf3954424, 0x34f6, 0x11d1, 0x82, 0x1d, 0x0, 0x0, 0xf8, 0x30, 0x2, 0x12

#define M_NOCONNECT     0x328c7247, 0x2b0d, 0x11d2, 0xa4, 0xd9, 0x0, 0x60, 0x8, 0xb, 0xa6, 0x34

// {A7DF70A1-6452-11d2-B371-006097170118}
DEFINE_GUIDSTRUCT("A7DF70A1-6452-11d2-B371-006097170118", GUID_SEND_VIDEO_STANDARD);
#define GUID_SEND_VIDEO_STANDARD DEFINE_GUIDNAMED(GUID_SEND_VIDEO_STANDARD)


typedef struct {
    INTERFACE _vddInterface;

    // CSpciIo* returned -- declared as PVOID to avoid excessive dependency on HAL classes
    PVOID pSpciIo;
} SPCIIOINTERFACE;

// interface to obtain master CSpciIo object -- uses SPCIIOINTERFACE structure
// {5DA0C940-0884-11d3-A4D9-0060080BA634}
DEFINE_GUIDSTRUCT("5DA0C940-0884-11d3-A4D9-0060080BA634", GUID_SPCIIO_INTERFACE);
#define GUID_SPCIIO_INTERFACE DEFINE_GUIDNAMED(GUID_SPCIIO_INTERFACE)



// {548F5540-F3DC-11d2-A4D9-0060080BA634}
DEFINE_GUIDSTRUCT("548F5540-F3DC-11d2-A4D9-0060080BA634", PINNAME_DEBI_IN);
#define PINNAME_DEBI_IN DEFINE_GUIDNAMED(PINNAME_DEBI_IN)


// {0896cda0-393f-11d3-8a11-00609405306e}
DEFINE_GUIDSTRUCT("0896cda0-393f-11d3-8a11-00609405306e", GUID_IF_DISPATCHER);
#define GUID_IF_DISPATCHER DEFINE_GUIDNAMED(GUID_IF_DISPATCHER)

// {4cadb500-4419-11d3-91b2-00c04f81b56b}
DEFINE_GUIDSTRUCT("4cadb500-4419-11d3-91b2-00c04f81b56b", GUID_IF_INTERFACE);
#define GUID_IF_INTERFACE DEFINE_GUIDNAMED(GUID_IF_INTERFACE)

// {a2487ce0-5b05-11d3-91b2-00c04f81b56b}
DEFINE_GUIDSTRUCT("a2487ce0-5b05-11d3-91b2-00c04f81b56b", GUID_SEND_DIAG_INFO);
#define GUID_SEND_DIAG_INFO DEFINE_GUIDNAMED(GUID_SEND_DIAG_INFO)


#endif //_MEDGUID_H_
