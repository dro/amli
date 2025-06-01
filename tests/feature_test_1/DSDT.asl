DefinitionBlock ("", "DSDT", 2, "Intel", "Example", 0x00000001)
{
    Name (TSFI, Zero)
    Name (WHL0, 0x10)
    Name (WHL1, Zero)
    While (Ones)
    {
        If (WHL0)
        {
            WHL0--
            WHL1++
        }
        Else
        {
            Break
        }
    }

    If ((WHL0 != Zero))
    {
        TSFI = One
    }
    ElseIf ((WHL1 != 0x10))
    {
        TSFI = 0x02
    }

    Name (CMP0, 0x10)
    Name (CMP1, 0x04)
    If ((CMP0 < CMP1))
    {
        TSFI = 0x03
    }
    ElseIf ((CMP0 == CMP1))
    {
        TSFI = 0x04
    }
    ElseIf ((CMP0 <= CMP1))
    {
        TSFI = 0x05
    }
    ElseIf ((CMP0 <= CMP1))
    {
        TSFI = 0x06
    }
    ElseIf (!(CMP0 >= 0x10))
    {
        TSFI = 0x07
    }
    ElseIf (((CMP0 < CMP1) || (CMP0 < 0x06)))
    {
        TSFI = 0x08
    }

    Name (CMP2, "Hello world")
    Name (CMP3, "Hello monde")
    Name (CMP4, "Hello world")
    Name (CMP5, "Hello world and people")
    If ((CMP2 <= CMP3))
    {
        TSFI = 0x09
    }
    ElseIf ((CMP2 == CMP3))
    {
        TSFI = 0x0A
    }
    ElseIf ((CMP2 != CMP4))
    {
        TSFI = 0x0B
    }
    ElseIf ((CMP2 < CMP4))
    {
        TSFI = 0x0C
    }
    ElseIf ((CMP4 >= CMP5))
    {
        TSFI = 0x0D
    }

    Name (CVS0, "DEADBEEF")
    Name (CVS1, "1000")
    If ((0xDEADBEEF != CVS0))
    {
        TSFI = 0x0E
    }
    ElseIf ((0x1000 != CVS1))
    {
        TSFI = 0x0F
    }
    ElseIf ((0xFEEDBEEF < CVS0))
    {
        TSFI = 0x10
    }
    ElseIf ((0x1000 != "1000"))
    {
        TSFI = 0x11
    }

    Name (CVS3, Zero)
    CVS3 = (CVS0 + CVS1) /* \CVS1 */
    If ((CVS3 != 0xDEADCEEF))
    {
        TSFI = 0x12
    }

    Name (CVS4, "")
    CVS4 = Buffer (0x04)
        {
             0xDE, 0xAD, 0xBE, 0xEF                           // ....
        }
    Name (SZF0, "hello")
    If ((SizeOf (SZF0) != 0x05))
    {
        TSFI = 0x13
    }

    Name (EXC0, Buffer (0x08)
    {
         0x30, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00   // 0.@.....
    })
    Name (EXC1, 0x04D2)
    Name (EXC2, 0x1234)
    Name (BUF0, Buffer (0x07)
    {
         0x44, 0x45, 0x46, 0x00, 0x44, 0x55, 0x00         // DEF.DU.
    })
    Name (EXC3, "")
    EXC3 = ToString (BUF0, Ones)
    Name (EXC4, "")
    EXC4 = "1234"
    Name (EXC5, "")
    EXC5 = ToDecimalString (BUF0)
    If ((EXC1 != 0x04D2))
    {
        TSFI = 0x13
    }
    ElseIf ((EXC2 != 0x1234))
    {
        TSFI = 0x14
    }
    ElseIf ((EXC3 != "DEF"))
    {
        TSFI = 0x15
    }
    ElseIf ((EXC4 != "1234"))
    {
        TSFI = 0x16
    }
    ElseIf ((EXC5 != "68,69,70,0,68,85,0"))
    {
        Debug = "EXC5 fail:"
        Debug = EXC5
        TSFI = 0x17
    }

    Local0 = (Zero + EXC3) /* \EXC3 */
    Name (DST0, Zero)
    CopyObject (RefOf (DST0), Arg0)
    Arg0 = 0x0400
    If ((DerefOf (RefOf (DST0)) != 0x0400))
    {
        TSFI = 0x18
    }

    Name (MTG0, Zero)

    Method (MTH1, 1, NotSerialized)
    {
        Arg0 = (DerefOf (Arg0) + 0x07D0)
        Return (Arg0)
    }

    MTH1 (RefOf (MTG0))
    If ((MTG0 != 0x07D0))
    {
        TSFI = 0x1A
    }

    Method (MTH2, 0, NotSerialized)
    {
        Return (RefOf (MTG0))
    }

    MTG0 = 0x0BB8
    If ((DerefOf (MTH2 ()) != 0x0BB8))
    {
        TSFI = 0x1B
    }

    Debug = Zero
    Name (XYZ0, 0x1000)
    Name (XYZ1, 0x0200)
    Name (STR0, "String initial value")
    Name (TEST, "Hello world!")
    Device (DEV0)
    {
        Name (_HID, "TESTDE00")  // _HID: Hardware ID
        Name (HI00, "100")
        Name (^HI01, "In parent scope")
    }

    If (Zero)
    {
        Name (CND0, "Conditional")
    }

    //
    // Test field creation.
    //

    OperationRegion(DEB0, SystemIO, 0x80, 0x2000)
	Field(DEB0, ByteAcc, NoLock, Preserve) {
		DBG8, 8,
        D4LO, 4,
        D4HI, 4,
        BNK0, 16,
        BNK1, 8,
        BIGF, 128,
        BIGD, 128
	}

    BankField(DEB0, BNK1, 0, ByteAcc, NoLock, Preserve) {
        Offset(0x30),
        FET0, 1,
        FET1, 1
    }

    Name(BUF9, Buffer{0xDE, 0xAD, 0xBE, 0xEF})
    CreateByteField(BUF9, 1, BF01)

    //
    // Test buffer field byte-granularity access.
    //
    Store(0xCC, BF01)
    If(BF01 != 0xCC) {
        TSFI = 0x1C
    }
    CreateDWordField(BUF9, 0, BF02)
    Store(Or(BF02, 0xFF), BF02)
    If(BF02 != 0xEFBECCFF) {
        TSFI = 0x1D
    }
    Store(Buffer{0xAA, 0xBB, 0xCC, 0xDD, 0xFF}, BF02)
    If(BF02 != 0xDDCCBBAA) {
        TSFI = 0x1E
    }
    Store(Buffer{0x11, 0x22}, BF02)
    If(BF02 != 0x00002211) {
        TSFI = 0x1F
    }
    Store(Buffer{0x13, 0x14}, BF01)
    If(BF02 != 0x00001311) {
        TSFI = 0x20
    }
    
    //
    // TODO: Test buffer field bit-granularity access.
    //

    //
    // Test packages.
    //
    Name(BLB0, 4)
    Name(PKG0, Package(BLB0){0x10, 0x20, \BLB0})

    //
    // Test buffer/string index.
    //
    Name(LOLX, 1)
    Store(DerefOf(Index("World", 2)), LOLX)
    If(LOLX != 0x72) {
        TSFI = 0x21
    }
    Store(1, Index("World", 2))

    //
    // test package index.
    //
    Store(DerefOf(Index(PKG0, 1)), LOLX)
    If(LOLX != 0x20) {
        TSFI = 0x22
    }
    Store(1337, Index(PKG0, 2))
    If(DerefOf(Index(PKG0, 2)) != 1337) {
        TSFI = 0x23
    }

    //
    // Test ObjectType.
    //
    If(ObjectType(PKG0) != 4) {
        TSFI = 0x24
    }
    If(ObjectType(BF02) != 14) {
        TSFI = 0x25
    }
    If(ObjectType(RefOf(BF02)) != 14) {
        TSFI = 0x26
    }
    If(ObjectType(Index("World", 2)) != 14) {
        TSFI = 0x27
    }
    If(ObjectType(RefOf(LOLX)) != 1) {
        TSFI = 0x28
    }
    If(ObjectType(Index(PKG0, 0)) != 1) {
        TSFI = 0x29
    }

    //
    // Scope testing.
    //
    Device(DEV1) {
        Name(INV0, 133)
        Scope(_SB) {
            If(CondRefOf(INV0)) {
                TSFI = 0x2A
            }
        }
        Device(DEV2) {
            Device(DEV3) {
                Name(^^^DEV1.TSTX, 190)
            }
        }
    }

    //
    // Test field writes.
    //
    Store(0xAABBCCDD, BNK0)
    Store(BNK0, D4LO)
    Store(0xCC, D4HI)

    //
    // Test processor object.
    //
    Processor (PR00, 0x00, 0x00001810, 0x06){}

    //
    // Test temporary objects inside of method scope.
    //
    Method(TMP1) {
        Name(TEST, 1)
        Device(DEV0) {
            Name(TEST, 2)
        }
        Store(DEV0.TEST, D4HI)
    }
    Method(TMP0) {
        Name(TEST, 1)
        Device(DEV0) {
            Name(TEST, 2)
        }
        Store(DEV0.TEST, D4HI)
        TMP1()
    }
    TMP0()

    //
    // Test ThermalZone.
    //
    ThermalZone(TMZ0) {
        Name(BLEH, 1)
    }

    //
    // Test timer.
    //
    Name(TIM0, 1)
    Name(TIM1, 0)
    Store(Timer, TIM0)
    Store(Timer, TIM1) 
    If(TIM0 > TIM1) {
        TSFI = 0x2B
    }

    //
    // Test IndexFields.
    //
    OperationRegion (RTCO, SystemIO, 0x72, 0xFF)
    Field (RTCO, ByteAcc, NoLock, Preserve)
    {
        CIND,   8, 
        CDAT,   8
    }
    IndexField (CIND, CDAT, ByteAcc, NoLock, Preserve)
    {
        Offset (0x5A), 
        OSTP,   8,
        OSTD,   8,
    }
    Store(0xCC, OSTP)
    Debug = CIND
    Debug = CDAT
    Store(0xDD, OSTD)
    Debug = CIND
    Debug = CDAT

    //
    // Test Notify.
    //
    Notify(DEV1, 0x0C)

    //
    // Test stall and sleep.
    //
    Stall(100)
    Sleep(1)

    //
    // Test mutex.
    //
    Mutex(MTX0)
    Acquire(MTX0, 100)
    Release(MTX0)

    //
    // Test event.
    //
    Event(EVT0)
    Signal(EVT0)
    Wait(EVT0, 100)

    //
    // Test mid.
    //
    Name(BUF2, Buffer{1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
    Name(MID0, Buffer{})
    Mid(BUF2, 4, 9, MID0)
    If(DerefOf(Index(MID0, 0)) != 5) {
        TSFI = 0x2C
    }
    Mid(BUF2, 9, 32, MID0)
    If(DerefOf(Index(MID0, 0)) != 10) {
        TSFI = 0x2C
    }

    //
    // Test for copy semantics bug.
    //
    Name(BUF3, Buffer{0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC})
    Name(BUF4, Buffer{})
    Store(BUF3, BUF4)
    Store(0xFF, Index(BUF4, 0))
    Debug = BUF3
    Debug = BUF4
    If(DerefOf(Index(BUF3, 0)) != 0xCC) {
        TSFI = 0x2D
    }

    //
    // Test BCD conversion.
    //
    If(ToBCD(1902) != 0x1902) {
        TSFI = 0x2E
    }
    If(FromBCD(0x1902) != 1902) {
        TSFI = 0x2F
    }
    Debug = ToBCD(1902)

    //
    // Test concatenate.
    //
    // If(Concatenate("hello ", \DEV0) != "hello [Device Object]") { /* ACPI doesn't follow spec for the type-name of device */
    //     TSFI = 0x30
    // }
    Debug = Concatenate("hello ", \DEV1.DEV2)
    If((Concatenate(0xFF, 0xCC) != Buffer{0xFF, 0, 0, 0, 0, 0, 0, 0, 0xCC, 0, 0, 0, 0, 0, 0, 0})
        && (Concatenate(0xFF, 0xCC) != Buffer{0xFF, 0, 0, 0, 0xCC, 0, 0, 0}))
    {
        TSFI = 0x31
    }
    If(Concatenate(Buffer{0xFF, 0xCC}, Buffer{0xAA, 0xBB}) != Buffer{0xFF, 0xCC, 0xAA, 0xBB}) {
        TSFI = 0x32
    }

    //
    // Test ConcatenateResTemplate.
    //
    Name (RES0, ResourceTemplate ()
    {
        GpioInt (Level, ActiveLow, ExclusiveAndWake, PullDefault, 0x0000,
            "\\_SB.PCI0.GPI0", 0x00, ResourceConsumer, ,
            )
            {   // Pin list
                0x0000
            }
    })
    Name (RES1, ResourceTemplate ()
    {
        GpioInt (Level, ActiveLow, ExclusiveAndWake, PullDefault, 0x0000,
            "\\_SB.PCI1.GPI1", 0x00, ResourceConsumer, ,
            )
            {   // Pin list
                0x0033
            }
    })
    Name(RESC, Buffer(0){}) 
    Store(ConcatenateResTemplate(RES0, RES1), RESC)
    If(SizeOf(RESC) != (SizeOf(RES0) + SizeOf(RES1) - 2)) {
        TSFI = 0x33
    }

    //
    // Test match.
    //
    Name(P1, Package(){1981, 1983, 1985, 1987, 1989, 1990, 1991, 1993, 1995, 1997, 1999, 2001})
    If(Match(P1, MEQ, 1993, MTR, 0, 0) != 7) { // -> 7, since P1[7] == 1993
        TSFI = 0x34
    }
    If(Match(P1, MEQ, 1984, MTR, 0, 0) != Ones) { // -> ONES (not found)
        TSFI = 0x35
    } 
    If(Match(P1, MGT, 1984, MLE, 2000, 0) != 2) { // -> 2, since P1[2]>1984 and P1[2]<=2000
        TSFI = 0x36
    }
    If(Match(P1, MGT, 1984, MLE, 2000, 3) != 3){ // -> 3, first match at or past Start
        TSFI = 0x37
    }

    //
    // Test more field access specifiers.
    //
    Field (RTCO, WordAcc, NoLock, Preserve) {
        FLD0, 16,
        AccessAs(DWordAcc, AttribQuick),
        FLD1, 16,
        AccessAs(ByteAcc, AttribBytes(4)),
        FLD2, 32,
        AccessAs(ByteAcc, AttribRawBytes(32)),
        FLD3, 32,
    }
    Add(10, 0, FLD1)

    //
    // Test for native code invoking AML method.
    //
    Method(NATM, 1) {
        Return(Arg0 + 1337)
    }

    //
    // Test invoking native method.
    //
    Name(OSYS, 0)
    If(CondRefOf(\_OSI)) {
        If (_OSI ("Windows 2013")) {
            OSYS = 0x07DD
        }
        If (_OSI ("Windows 2015")) {
            OSYS = 0x07DF
        }
    }
    Debug = OSYS

    //
    // Test dynamically loading a table from AML.
    //
    Scope(_SB) {
        Device(PCI0) {
            Device(SAT0) {
                Name(BLEH, 0)
            }
            Device(SAT1) {
                Name(BLEH, 0)
            }
        }
    }
    Name(DSSP, 0)
    Name(FHPP, 0)
    Name(DCTB, Buffer{
        0x53, 0x53, 0x44, 0x54, 0x6D, 0x03, 0x00, 0x00, 0x01, 0x3E, 0x53, 0x61,
        0x74, 0x61, 0x52, 0x65, 0x53, 0x61, 0x74, 0x61, 0x54, 0x61, 0x62, 0x6C,
        0x00, 0x10, 0x00, 0x00, 0x49, 0x4E, 0x54, 0x4C, 0x11, 0x07, 0x12, 0x20,
        0x10, 0x46, 0x09, 0x5C, 0x00, 0x08, 0x53, 0x54, 0x46, 0x45, 0x11, 0x0A,
        0x0A, 0x07, 0x10, 0x06, 0x00, 0x00, 0x00, 0x00, 0xEF, 0x08, 0x53, 0x54,
        0x46, 0x44, 0x11, 0x0A, 0x0A, 0x07, 0x90, 0x06, 0x00, 0x00, 0x00, 0x00,
        0xEF, 0x08, 0x46, 0x5A, 0x54, 0x46, 0x11, 0x0A, 0x0A, 0x07, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xF5, 0x08, 0x44, 0x43, 0x46, 0x4C, 0x11, 0x0A,
        0x0A, 0x07, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB1, 0x08, 0x53, 0x43,
        0x42, 0x46, 0x11, 0x03, 0x0A, 0x15, 0x08, 0x43, 0x4D, 0x44, 0x43, 0x00,
        0x14, 0x42, 0x04, 0x47, 0x54, 0x46, 0x42, 0x0A, 0x77, 0x43, 0x4D, 0x44,
        0x43, 0x0A, 0x38, 0x60, 0x5B, 0x13, 0x53, 0x43, 0x42, 0x46, 0x60, 0x0A,
        0x38, 0x43, 0x4D, 0x44, 0x58, 0x77, 0x43, 0x4D, 0x44, 0x43, 0x0A, 0x07,
        0x60, 0x8C, 0x53, 0x43, 0x42, 0x46, 0x72, 0x60, 0x01, 0x00, 0x41, 0x30,
        0x30, 0x31, 0x70, 0x68, 0x43, 0x4D, 0x44, 0x58, 0x70, 0x69, 0x41, 0x30,
        0x30, 0x31, 0x75, 0x43, 0x4D, 0x44, 0x43, 0x10, 0x41, 0x2B, 0x5C, 0x2F,
        0x03, 0x5F, 0x53, 0x42, 0x5F, 0x50, 0x43, 0x49, 0x30, 0x53, 0x41, 0x54,
        0x30, 0x08, 0x52, 0x45, 0x47, 0x46, 0x01, 0x14, 0x12, 0x5F, 0x52, 0x45,
        0x47, 0x02, 0xA0, 0x0B, 0x93, 0x68, 0x0A, 0x02, 0x70, 0x69, 0x52, 0x45,
        0x47, 0x46, 0x08, 0x54, 0x4D, 0x44, 0x30, 0x11, 0x03, 0x0A, 0x14, 0x8A,
        0x54, 0x4D, 0x44, 0x30, 0x00, 0x50, 0x49, 0x4F, 0x30, 0x8A, 0x54, 0x4D,
        0x44, 0x30, 0x0A, 0x04, 0x44, 0x4D, 0x41, 0x30, 0x8A, 0x54, 0x4D, 0x44,
        0x30, 0x0A, 0x08, 0x50, 0x49, 0x4F, 0x31, 0x8A, 0x54, 0x4D, 0x44, 0x30,
        0x0A, 0x0C, 0x44, 0x4D, 0x41, 0x31, 0x8A, 0x54, 0x4D, 0x44, 0x30, 0x0A,
        0x10, 0x43, 0x48, 0x4E, 0x46, 0x14, 0x32, 0x5F, 0x47, 0x54, 0x4D, 0x00,
        0x70, 0x0A, 0x78, 0x50, 0x49, 0x4F, 0x30, 0x70, 0x0A, 0x14, 0x44, 0x4D,
        0x41, 0x30, 0x70, 0x0A, 0x78, 0x50, 0x49, 0x4F, 0x31, 0x70, 0x0A, 0x14,
        0x44, 0x4D, 0x41, 0x31, 0x7D, 0x43, 0x48, 0x4E, 0x46, 0x0A, 0x05, 0x43,
        0x48, 0x4E, 0x46, 0xA4, 0x54, 0x4D, 0x44, 0x30, 0x14, 0x06, 0x5F, 0x53,
        0x54, 0x4D, 0x03, 0x5B, 0x82, 0x44, 0x05, 0x53, 0x50, 0x54, 0x30, 0x08,
        0x5F, 0x41, 0x44, 0x52, 0x0B, 0xFF, 0xFF, 0x14, 0x45, 0x04, 0x5F, 0x47,
        0x54, 0x46, 0x00, 0x70, 0x00, 0x43, 0x4D, 0x44, 0x43, 0xA0, 0x14, 0x91,
        0x44, 0x53, 0x53, 0x50, 0x46, 0x48, 0x50, 0x50, 0x47, 0x54, 0x46, 0x42,
        0x53, 0x54, 0x46, 0x44, 0x0A, 0x06, 0xA1, 0x0B, 0x47, 0x54, 0x46, 0x42,
        0x53, 0x54, 0x46, 0x45, 0x0A, 0x06, 0x47, 0x54, 0x46, 0x42, 0x46, 0x5A,
        0x54, 0x46, 0x00, 0x47, 0x54, 0x46, 0x42, 0x44, 0x43, 0x46, 0x4C, 0x00,
        0xA4, 0x53, 0x43, 0x42, 0x46, 0x5B, 0x82, 0x46, 0x05, 0x53, 0x50, 0x54,
        0x31, 0x08, 0x5F, 0x41, 0x44, 0x52, 0x0C, 0xFF, 0xFF, 0x01, 0x00, 0x14,
        0x45, 0x04, 0x5F, 0x47, 0x54, 0x46, 0x00, 0x70, 0x00, 0x43, 0x4D, 0x44,
        0x43, 0xA0, 0x14, 0x91, 0x44, 0x53, 0x53, 0x50, 0x46, 0x48, 0x50, 0x50,
        0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x44, 0x0A, 0x06, 0xA1, 0x0B,
        0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x45, 0x0A, 0x06, 0x47, 0x54,
        0x46, 0x42, 0x46, 0x5A, 0x54, 0x46, 0x00, 0x47, 0x54, 0x46, 0x42, 0x44,
        0x43, 0x46, 0x4C, 0x00, 0xA4, 0x53, 0x43, 0x42, 0x46, 0x5B, 0x82, 0x46,
        0x05, 0x53, 0x50, 0x54, 0x32, 0x08, 0x5F, 0x41, 0x44, 0x52, 0x0C, 0xFF,
        0xFF, 0x02, 0x00, 0x14, 0x45, 0x04, 0x5F, 0x47, 0x54, 0x46, 0x00, 0x70,
        0x00, 0x43, 0x4D, 0x44, 0x43, 0xA0, 0x14, 0x91, 0x44, 0x53, 0x53, 0x50,
        0x46, 0x48, 0x50, 0x50, 0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x44,
        0x0A, 0x06, 0xA1, 0x0B, 0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x45,
        0x0A, 0x06, 0x47, 0x54, 0x46, 0x42, 0x46, 0x5A, 0x54, 0x46, 0x00, 0x47,
        0x54, 0x46, 0x42, 0x44, 0x43, 0x46, 0x4C, 0x00, 0xA4, 0x53, 0x43, 0x42,
        0x46, 0x5B, 0x82, 0x46, 0x05, 0x53, 0x50, 0x54, 0x33, 0x08, 0x5F, 0x41,
        0x44, 0x52, 0x0C, 0xFF, 0xFF, 0x03, 0x00, 0x14, 0x45, 0x04, 0x5F, 0x47,
        0x54, 0x46, 0x00, 0x70, 0x00, 0x43, 0x4D, 0x44, 0x43, 0xA0, 0x14, 0x91,
        0x44, 0x53, 0x53, 0x50, 0x46, 0x48, 0x50, 0x50, 0x47, 0x54, 0x46, 0x42,
        0x53, 0x54, 0x46, 0x44, 0x0A, 0x06, 0xA1, 0x0B, 0x47, 0x54, 0x46, 0x42,
        0x53, 0x54, 0x46, 0x45, 0x0A, 0x06, 0x47, 0x54, 0x46, 0x42, 0x46, 0x5A,
        0x54, 0x46, 0x00, 0x47, 0x54, 0x46, 0x42, 0x44, 0x43, 0x46, 0x4C, 0x00,
        0xA4, 0x53, 0x43, 0x42, 0x46, 0x5B, 0x82, 0x46, 0x05, 0x53, 0x50, 0x54,
        0x34, 0x08, 0x5F, 0x41, 0x44, 0x52, 0x0C, 0xFF, 0xFF, 0x04, 0x00, 0x14,
        0x45, 0x04, 0x5F, 0x47, 0x54, 0x46, 0x00, 0x70, 0x00, 0x43, 0x4D, 0x44,
        0x43, 0xA0, 0x14, 0x91, 0x44, 0x53, 0x53, 0x50, 0x46, 0x48, 0x50, 0x50,
        0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x44, 0x0A, 0x06, 0xA1, 0x0B,
        0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x45, 0x0A, 0x06, 0x47, 0x54,
        0x46, 0x42, 0x46, 0x5A, 0x54, 0x46, 0x00, 0x47, 0x54, 0x46, 0x42, 0x44,
        0x43, 0x46, 0x4C, 0x00, 0xA4, 0x53, 0x43, 0x42, 0x46, 0x5B, 0x82, 0x46,
        0x05, 0x53, 0x50, 0x54, 0x35, 0x08, 0x5F, 0x41, 0x44, 0x52, 0x0C, 0xFF,
        0xFF, 0x05, 0x00, 0x14, 0x45, 0x04, 0x5F, 0x47, 0x54, 0x46, 0x00, 0x70,
        0x00, 0x43, 0x4D, 0x44, 0x43, 0xA0, 0x14, 0x91, 0x44, 0x53, 0x53, 0x50,
        0x46, 0x48, 0x50, 0x50, 0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x44,
        0x0A, 0x06, 0xA1, 0x0B, 0x47, 0x54, 0x46, 0x42, 0x53, 0x54, 0x46, 0x45,
        0x0A, 0x06, 0x47, 0x54, 0x46, 0x42, 0x46, 0x5A, 0x54, 0x46, 0x00, 0x47,
        0x54, 0x46, 0x42, 0x44, 0x43, 0x46, 0x4C, 0x00, 0xA4, 0x53, 0x43, 0x42,
        0x46
        }
    )
    If(Load(DCTB)) {
        Debug = "Loaded"
    } Else {
        Debug = "Failed to load"
    }

    //
    // Test dynamically loading an ACPI table.
    //
    Device(TRES) {
        Name(SUCS, 0)
        Device(NWTB) { }
    }
    If(LoadTable("SSDT", "PmRef", "LakeTiny", "\\TRES.NWTB", "\\TRES.SUCS", 1)) {
        Debug = "Loaded 2"
    } Else {
        Debug = "Failed to load 2"
    }
    If(\TRES.SUCS != 1) {
        TSFI = 0x38
    }

    //
    // Test PCI configuration space access.
    //
    Device(PC00) { // Root bus
        Name (_HID, EisaId ("PNP0A08") /* PCI Express Bus */)  // _HID: Hardware ID
        Name (_CID, EisaId ("PNP0A03") /* PCI Bus */)  // _CID: Compatible ID
        Name(_SEG, 0)
        Name(_BBN, 0)
        Name(_ADR, 0)
        Device(DV00) {
            Name(_ADR, 0x50002) // Final device relative to the previous bridge. 
            OperationRegion (CFG, PCI_Config, Zero, 0x100)
            Field(CFG, AnyAcc, NoLock, Preserve) {
                VID0, 16,
                DID0, 16,
            }
        }
        OperationRegion (HBUS, PCI_Config, Zero, 0x0100)
        Field (HBUS, DWordAcc, NoLock, Preserve)
        {
            Offset (0x40), 
            EPEN,   1, 
                ,   11, 
            EPBR,   20, 
            Offset (0x48), 
            MHEN,   1, 
                ,   14, 
            MHBR,   17, 
            Offset (0x50), 
            GCLK,   1, 
            Offset (0x54), 
            D0EN,   1, 
            D1F2,   1, 
            D1F1,   1, 
            D1F0,   1, 
                ,   9, 
            D6F0,   1, 
            Offset (0x60), 
            PXEN,   1, 
            PXSZ,   3, 
                ,   22, 
            PXBR,   6, 
            Offset (0x68), 
            DIEN,   1, 
                ,   11, 
            DIBR,   20, 
            Offset (0x70), 
                ,   20, 
            MEBR,   12, 
            Offset (0x80), 
            PMLK,   1, 
                ,   3, 
            PM0H,   2, 
            Offset (0x81), 
            PM1L,   2, 
                ,   2, 
            PM1H,   2, 
            Offset (0x82), 
            PM2L,   2, 
                ,   2, 
            PM2H,   2, 
            Offset (0x83), 
            PM3L,   2, 
                ,   2, 
            PM3H,   2, 
            Offset (0x84), 
            PM4L,   2, 
                ,   2, 
            PM4H,   2, 
            Offset (0x85), 
            PM5L,   2, 
                ,   2, 
            PM5H,   2, 
            Offset (0x86), 
            PM6L,   2, 
                ,   2, 
            PM6H,   2, 
            Offset (0x87), 
            Offset (0xA8), 
                ,   20, 
            TUUD,   19, 
            Offset (0xBC), 
                ,   20, 
            TLUD,   12, 
            Offset (0xC8), 
                ,   7, 
            HTSE,   1
        }

        Method (GPCB, 0, Serialized)
        {
            Return((PXBR << 0x1A))
        }

        Method (SAVC, 0, Serialized) {
            OperationRegion (MCRC, SystemMemory, (GPCB () + 0x000B833C), 0x04)
        }
    }
    Store(0xCC, \PC00.DV00.VID0)
    PC00.SAVC() // test

    //
    // Test special global lock mutex. 
    //
    If(Acquire(_GL, 0) == 0) {
        Acquire(_GL, 0)
        Debug = "Holding _GL."
        Name(HGL0, 1)
        Release(_GL)
        Release(_GL)
    } Else {
        Debug = "Failed to acquire _GL."
        Name(NAC0, 1)
    }

    //
    // Test global lock protected field.
    //
    OperationRegion (PKBS, SystemIO, 0x60, 0x05)
    Field (PKBS, ByteAcc, Lock, Preserve)
    {
        PKBD,   8, 
        Offset (0x02), 
        Offset (0x03), 
        Offset (0x04), 
        PKBC,   8
    }
    Store(0xCC, PKBD)

    //
    // Testing package to package stores.
    //
    Method(STN0, 0) {
        Store("string", Local0)
        Store(PKBD, Local1)
        Store(Buffer{1, 2, 3}, Local2)
        
        // Test package to package stores.
        Name(PKGZ, Package{1, 2, 3, 4})
        Store(Package{0xFF}, PKGZ)
        If(DerefOf(Index(PKGZ, 0)) != 0xFF || SizeOf(PKGZ) != 1) {
            TSFI = 0x39
        }

        // Packages should be copied, not referenced.
        Name(PKG0, Package{1, 2, 3, 4})
        Name(PKG1, Package{0})
        Store(PKG0, PKG1) 
        Store(0xCC, Index(PKG1, 1))
        If(DerefOf(Index(PKG0, 1)) != 2 || DerefOf(Index(PKG1, 1)) != 0xCC) {
            TSFI = 0x3A
        }

        Debug = PKGZ
    }
    STN0()

    //
    // Test method-scoped mutex acquisition/automatic release.
    //
    Method(MTS0, 0) {
        Acquire(\_GL, 0xFFFF)
        Acquire(\_GL, 0xFFFF)
        Release(\_GL)
        Release(\_GL)
    }
    Method(MTS1, 0) {
        Acquire(\_GL, 0xFFFF)
        Acquire(\_GL, 0xFFFF)
        Acquire(\_GL, 0xFFFF)
        MTS0()
        Release(\_GL)
    }
    MTS1()

    //
    // Test automatic release in global scope.
    //
    Acquire(MTX0, 0xFFFF)

    //
    // Test field access beyond 64bit size.
    //
    OperationRegion(OPR0, SystemMemory, 0x10000, 0x2000)
	Field(OPR0, DWordAcc, NoLock, Preserve) {
        BIG0, 256,
        BIG1, 256
	}
    Store(Ones, BIG0)
    Debug = BIG0
    Store(ToBuffer("this is a big string treat it like a buffer"), BIG1)
    Debug = BIG1

    //
    // Test IPMI access.
    //
    Device (PMI0)
    {
        Name (_HID, "ACPI000D" /* Power Meter Device */)  // _HID: Hardware ID
        OperationRegion (POWR, IPMI, 0x2C02, 0x0100)
        Field (POWR, BufferAcc, Lock, Preserve)
        {
            AccessAs (BufferAcc, 0x01), 
            GPOW,   8, 
            GCAP,   8
        }
    }
    Name(IPMB, Buffer{0, 5, 1, 2, 3, 4, 5})
    Store(IPMB, PMI0.GPOW)
    Store(PMI0.GPOW, IPMB)
    Debug = "IPMI GPOW readback: "
    Debug = IPMB

    //
    // Test SMBUS access.
    //
    Device(SMB0) {
        Method(_REG, 2) {
            Debug = "_REG notification."
            Debug = Concatenate(Concatenate(Concatenate("_REG notification: ", Arg0)," = "), Arg1)
        }
        OperationRegion(SBD0, SMBus, 0x0B00, 0x0100)
        Field(SBD0, BufferAcc, NoLock, Preserve)
        {
            AccessAs(BufferAcc, SMBWord),   // Use the SMBWord protocol for the following...
            MFGA, 8,                        // ManufacturerAccess() [command value 0x00]
            RCAP, 8,                        // RemainingCapacityAlarm() [command value 0x01]
            Offset(0x08),                   // Skip to command value 0x08...
            BTMP, 8,                        // Temperature() [command value 0x08]
            Offset(0x20),                   // Skip to command value 0x20...
            AccessAs(BufferAcc, SMBBlock),  // Use the SMBBlock protocol for the following...
            MFGN, 8,                        // ManufacturerName() [command value 0x20]
            DEVN, 8                         // DeviceName() [command value 0x21]
        }
        Name(BUFF, Buffer(34){})            // Create SMBus data buffer as BUFF
        CreateByteField(BUFF, 0x00, OB1)    // OB1 = Status (Byte)
        CreateByteField(BUFF, 0x01, OB2)    // OB2 = Length (Byte)
        CreateWordField(BUFF, 0x02, OB3)    // OB3 = Data (Word - Bytes 2 & 3)
        CreateField(BUFF, 0x10, 256, OB4)   // OB4 = Data (Block - Bytes 2-33)
    }
    Store(SMB0.BTMP, SMB0.BUFF) // Invoke Read Word transaction
    Debug = RefOf(SMB0)
    Debug = RefOf(TRES.SUCS)

    //
    // Test device initialization.
    //
    Method(_INI) {
        Debug = "Root _INI called!"
    }
    Name(DVC0, 0)
    Scope(SMB0) {
        Device(DVX0) {
            Method(_STA) {
                Return(0xFF)
            }
            Method(_INI) {
                Debug = "DBX0._INI called!"
            }
        }
    }
    Name(DVC1, 0)
    Device(DIS0) {
        Method(_STA) {
            Return(0x8) // Not present, but functional. should skip this _INI but keep processing children.
        }
        Method(_INI) {
            Debug = "!! DIS0._INI called, this should not happen !!"
            TSFI = 0x3B
        }
        Device(ENB0) {
            Method(_STA) {
                Return(0xFF)
            }
            Method(_INI) {
                Debug = "ENB0._INI called!"
            }
        }
    }

    //
    // Test field connections.
    //
    Device(CONX) {
        OperationRegion(TOP1, GenericSerialBus, 0x00, 0x100)
        Name (I2C, ResourceTemplate(){
            I2CSerialBusV2(0x5a,,100000,, "\\_SB.I2C",,,,,RawDataBuffer(){1,6})
        })
        Field(TOP1, BufferAcc, NoLock, Preserve) {
            Connection(I2C),                          // Specify connection resource information
            AccessAs(BufferAcc, AttribWord),          // Use the GenericSerialBus
            FLD0, 8,                                  // Virtual register at command value 0.
            FLD1, 8,                                  // Virtual register at command value 1.
        }
        Field(TOP1, BufferAcc, NoLock, Preserve) {
            Connection(I2CSerialBusV2(0x5b,,100000,, "\\_SB.I2C",,,,,RawDataBuffer(){3,9})),
            AccessAs(BufferAcc, AttribBytes (16)),
            FLD2, 8                                   // Virtual register at command value 0.
        }

        // Create the GenericSerialBus data buffer
        Name(GSBF, Buffer(34){})                     // Create GenericSerialBus data buffer as BUFF
        CreateByteField(GSBF, 0x00, STAT)            // STAT = Status (Byte)
        CreateWordField(GSBF, 0x02, DATA)            // DATA = Data (Word)
        Store(GSBF, FLD0)
        Store(FLD0, GSBF)
        Store(GSBF, FLD2)
        Store(FLD2, GSBF)
    }

    //
    // Test snapshot error handling.
    //
    External(\HOG.WASH, MethodObj)
    Method(SNP1) {
        \HOG.WASH()
    }
    Name(LNDR, Buffer{})
    Name(LNDX, Buffer{})
    Name(LNDP, Package{Buffer{}, "hello"})
    Method(SNP0) {
        Name(BLEH, "hello")
        Scope(\) {
            Name(LIVE, "it lives")
        }
        Store(ToBuffer("gaunderzoss"), LNDR)
        Store(ToBuffer("haundervoss"), Index(LNDP, 0))
        SNP1()
    }
    Method(SNPG) {
        Store(ToBuffer("xaunderposs"), LNDX)
        Store(0x45, Index(LNDX, 0))
        Debug = LNDR
        Debug = LNDX
        Debug = LNDP
    }
}

