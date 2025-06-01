//
// Encoding Value          Encoding Name             Encoding Group    Fixed List Arguments                                Variable List Arguments     IsExprOp
// ----------------------  ----------------------    ----------------  --------------------------------------------------  --------------------------  --------
// 0x00                    , ZeroOp                 , Data Object     , -                                                 , -                        , 0		
// 0x01                    , OneOp                  , Data Object     , -                                                 , -                        , 0		
// 0x06                    , AliasOp                , Term Object     , NameString NameString                             , -                        , 0		
// 0x08                    , NameOp                 , Term Object     , NameString DataRefObject                          , -                        , 0		
// 0x0A                    , BytePrefix             , Data Object     , ByteData                                          , -                        , 0		
// 0x0B                    , WordPrefix             , Data Object     , WordData                                          , -                        , 0		
// 0x0C                    , DWordPrefix            , Data Object     , DWordData                                         , -                        , 0		
// 0x0D                    , StringPrefix           , Data Object     , AsciiCharList NullChar                            , -                        , 0		
// 0x0E                    , QWordPrefix            , Data Object     , QWordData                                         , -                        , 0		
// 0x10                    , ScopeOp                , Term Object     , NameString                                        , TermList                 , 0		
// 0x11                    , BufferOp               , Term Object     , TermArg                                           , ByteList                 , 1		
// 0x12                    , PackageOp              , Term Object     , ByteData                                          , Package TermList         , 1		
// 0x13                    , VarPackageOp           , Term Object     , TermArg                                           , Package TermList         , 1		
// 0x14                    , MethodOp               , Term Object     , NameString ByteData                               , TermList                 , 0		
// 0x15                    , ExternalOp             , Name Object     , NameString ByteData ByteData                      , -                        , 0		
// 0x2E                    , DualNamePrefix         , Name Object     , NameSeg NameSeg                                   , -                        , 0		
// 0x2F                    , MultiNamePrefix        , Name Object     , ByteData NameSeg(N)                               , -                        , 0  		
// 0x30                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x31                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x32                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x33                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x34                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x35                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x36                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x37                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x38                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x39                    , DigitChar              , Name Object     , -                                                 , -        				 , 0		
// 0x41                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x42                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x43                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x44                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x45                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x46                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x47                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x48                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x49                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x4a                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x4b                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x4c                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x4d                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x4e                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x4f                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x50                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x51                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x52                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x53                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x54                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x55                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x56                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x57                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x58                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x59                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x5a                    , NameChar               , Name Object     , -                                                 , -     					 , 0		
// 0x5B                    , ExtOpPrefix            , -               , ByteData                                          , -                        , 0		
// 0x5B 0x01               , MutexOp                , Term Object     , NameString ByteData                               , -                        , 0		
// 0x5B 0x02               , EventOp                , Term Object     , NameString                                        , -                        , 0		
// 0x5B 0x12               , CondRefOfOp            , Term Object     , SuperName Target                                  , -                        , 1		
// 0x5B 0x13               , CreateFieldOp          , Term Object     , TermArg TermArg TermArg NameString                , -                        , 0		
// 0x5B 0x1F               , LoadTableOp            , Term Object     , TermArg TermArg TermArg TermArg TermArg TermArg   , -                        , 1		
// 0x5B 0x20               , LoadOp                 , Term Object     , NameString SuperName                              , -                        , 1		
// 0x5B 0x21               , StallOp                , Term Object     , TermArg                                           , -                        , 0		
// 0x5B 0x22               , SleepOp                , Term Object     , TermArg                                           , -                        , 0		
// 0x5B 0x23               , AcquireOp              , Term Object     , SuperName WordData                                , -                        , 1		
// 0x5B 0x24               , SignalOp               , Term Object     , SuperName                                         , -                        , 0		
// 0x5B 0x25               , WaitOp                 , Term Object     , SuperName TermArg                                 , -                        , 1		
// 0x5B 0x26               , ResetOp                , Term Object     , SuperName                                         , -                        , 0		
// 0x5B 0x27               , ReleaseOp              , Term Object     , SuperName                                         , -                        , 0		
// 0x5B 0x28               , FromBCDOp              , Term Object     , TermArg Target                                    , -                        , 1		
// 0x5B 0x29               , ToBCD                  , Term Object     , TermArg Target                                    , -                        , 1		
// 0x5B 0x30               , RevisionOp             , Data Object     , -                                                 , -                        , 0		
// 0x5B 0x31               , DebugOp                , Debug Object    , -                                                 , -                        , 0		
// 0x5B 0x32               , FatalOp                , Term Object     , ByteData DWordData TermArg                        , -                        , 0		
// 0x5B 0x33               , TimerOp                , Term Object     , -                                                 , -                        , 1		
// 0x5B 0x80               , OpRegionOp             , Term Object     , NameString ByteData TermArg TermArg               , -                        , 0		
// 0x5B 0x81               , FieldOp                , Term Object     , NameString ByteData                               , FieldList                , 0		
// 0x5B 0x82               , DeviceOp               , Term Object     , NameString                                        , TermList                 , 0		
// 0x5B 0x83               , ProcessorOp            , Term Object     , NameString ByteData DWordData ByteData            , ObjectList               , 0		
// 0x5B 0x84               , PowerResOp             , Term Object     , NameString ByteData WordData                      , TermList                 , 0		
// 0x5B 0x85               , ThermalZoneOp          , Term Object     , NameString                                        , TermList                 , 0		
// 0x5B 0x86               , IndexFieldOp           , Term Object     , NameString NameString ByteData                    , FieldList                , 0		
// 0x5B 0x87               , BankFieldOp            , Term Object     , NameString NameString TermArg ByteData            , FieldList                , 0		
// 0x5B 0x88               , DataRegionOp           , Term Object     , NameString TermArg TermArg TermArg                , -                        , 0		
// 0x5C                    , RootChar               , Name Object     , -                                                 , -                        , 0		
// 0x5E                    , ParentPrefixChar       , Name Object     , -                                                 , -                        , 0		
// 0x5F                    , NameChar-              , Name Object     , -                                                 , -                        , 0		
// 0x60                    , Local0Op               , Local Object    , -                                                 , -                        , 0		
// 0x61                    , Local1Op               , Local Object    , -                                                 , -                        , 0		
// 0x62                    , Local2Op               , Local Object    , -                                                 , -                        , 0		
// 0x63                    , Local3Op               , Local Object    , -                                                 , -                        , 0		
// 0x64                    , Local4Op               , Local Object    , -                                                 , -                        , 0		
// 0x65                    , Local5Op               , Local Object    , -                                                 , -                        , 0		
// 0x66                    , Local6Op               , Local Object    , -                                                 , -                        , 0		
// 0x67                    , Local7Op               , Local Object    , -                                                 , -                        , 0		
// 0x68                    , Arg0Op                 , Arg Object      , -                                                 , -                        , 0		
// 0x69                    , Arg1Op                 , Arg Object      , -                                                 , -                        , 0		
// 0x6A                    , Arg2Op                 , Arg Object      , -                                                 , -                        , 0		
// 0x6B                    , Arg3Op                 , Arg Object      , -                                                 , -                        , 0		
// 0x6C                    , Arg4Op                 , Arg Object      , -                                                 , -                        , 0		
// 0x6D                    , Arg5Op                 , Arg Object      , -                                                 , -                        , 0		
// 0x6E                    , Arg6Op                 , Arg Object      , -                                                 , -                        , 0		
// 0x70                    , StoreOp                , Term Object     , TermArg SuperName                                 , -                        , 1		
// 0x71                    , RefOfOp                , Term Object     , SuperName                                         , -                        , 1		
// 0x72                    , AddOp                  , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x73                    , ConcatOp               , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x74                    , SubtractOp             , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x75                    , IncrementOp            , Term Object     , SuperName                                         , -                        , 1		
// 0x76                    , DecrementOp            , Term Object     , SuperName                                         , -                        , 1		
// 0x77                    , MultiplyOp             , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x78                    , DivideOp               , Term Object     , TermArg TermArg Target Target                     , -                        , 1		
// 0x79                    , ShiftLeftOp            , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x7A                    , ShiftRightOp           , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x7B                    , AndOp                  , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x7C                    , NandOp                 , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x7D                    , OrOp                   , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x7E                    , NorOp                  , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x7F                    , XorOp                  , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x80                    , NotOp                  , Term Object     , TermArg Target                                    , -                        , 1		
// 0x81                    , FindSetLeftBitOp       , Term Object     , TermArg Target                                    , -                        , 1		
// 0x82                    , FindSetRightBitOp      , Term Object     , TermArg Target                                    , -                        , 1		
// 0x83                    , DerefOfOp              , Term Object     , TermArg                                           , -                        , 1		
// 0x84                    , ConcatResOp            , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x85                    , ModOp                  , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x86                    , NotifyOp               , Term Object     , SuperName TermArg                                 , -                        , 0		
// 0x87                    , SizeOfOp               , Term Object     , SuperName                                         , -                        , 1		
// 0x88                    , IndexOp                , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x89                    , MatchOp                , Term Object     , TermArg ByteData TermArg ByteData TermArg TermArg , -                        , 1		
// 0x8A                    , CreateDWordFieldOp     , Term Object     , TermArg TermArg NameString                        , -                        , 0		
// 0x8B                    , CreateWordFieldOp      , Term Object     , TermArg TermArg NameString                        , -                        , 0		
// 0x8C                    , CreateByteFieldOp      , Term Object     , TermArg TermArg NameString                        , -                        , 0		
// 0x8D                    , CreateBitFieldOp       , Term Object     , TermArg TermArg NameString                        , -                        , 0		
// 0x8E                    , ObjectTypeOp           , Term Object     , SuperName                                         , -                        , 1		
// 0x8F                    , CreateQWordFieldOp     , Term Object     , TermArg TermArg NameString                        , -                        , 0		
// 0x90                    , LandOp                 , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x91                    , LorOp                  , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x92                    , LnotOp                 , Term Object     , TermArg                                           , -                        , 1		
// 0x92 0x93               , LNotEqualOp            , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x92 0x94               , LLessEqualOp           , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x92 0x95               , LGreaterEqualOp        , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x93                    , LEqualOp               , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x94                    , LGreaterOp             , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x95                    , LLessOp                , Term Object     , TermArg TermArg                                   , -                        , 1		
// 0x96                    , ToBufferOp             , Term Object     , TermArg Target                                    , -                        , 1		
// 0x97                    , ToDecimalStringOp      , Term Object     , TermArg Target                                    , -                        , 1		
// 0x98                    , ToHexStringOp          , Term Object     , TermArg Target                                    , -                        , 1		
// 0x99                    , ToIntegerOp            , Term Object     , TermArg Target                                    , -                        , 1		
// 0x9C                    , ToStringOp             , Term Object     , TermArg TermArg Target                            , -                        , 1		
// 0x9D                    , CopyObjectOp           , Term Object     , TermArg SimpleName                                , -                        , 1		
// 0x9E                    , MidOp                  , Term Object     , TermArg TermArg TermArg Target                    , -                        , 1		
// 0x9F                    , ContinueOp             , Term Object     , -                                                 , -                        , 0		
// 0xA0                    , IfOp                   , Term Object     , TermArg                                           , TermList                 , 0		
// 0xA1                    , ElseOp                 , Term Object     , -                                                 , TermList                 , 0		
// 0xA2                    , WhileOp                , Term Object     , TermArg                                           , TermList                 , 0		
// 0xA3                    , NoopOp                 , Term Object     , -                                                 , -                        , 0		
// 0xA4                    , ReturnOp               , Term Object     , TermArg                                           , -                        , 0		
// 0xA5                    , BreakOp                , Term Object     , -                                                 , -                        , 0		
// 0xCC                    , BreakPointOp           , Term Object     , -                                                 , -                        , 0		
// 0xFF                    , OnesOp                 , Data Object     , -                                                 , -            			 , 0		
//

static const UINT8 AmlOpcodeFixedArgumentListSharedValueTable[] = {
     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,  AML_ARGUMENT_FIXED_TYPE_NAME_STRING,              AML_ARGUMENT_FIXED_TYPE_NAME_STRING, AML_ARGUMENT_FIXED_TYPE_DATA_REF_OBJECT,      
     AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,    AML_ARGUMENT_FIXED_TYPE_WORD_DATA,                AML_ARGUMENT_FIXED_TYPE_DWORD_DATA,  AML_ARGUMENT_FIXED_TYPE_ASCII_CHAR_LIST,      
     AML_ARGUMENT_FIXED_TYPE_NULL_CHAR,    AML_ARGUMENT_FIXED_TYPE_QWORD_DATA,               AML_ARGUMENT_FIXED_TYPE_NAME_STRING, AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,  AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,                AML_ARGUMENT_FIXED_TYPE_NAME_STRING, AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,      
     AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,    AML_ARGUMENT_FIXED_TYPE_NAME_SEG,                 AML_ARGUMENT_FIXED_TYPE_NAME_SEG,    AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,      
     AML_ARGUMENT_FIXED_TYPE_NAME_SEG_N,   AML_ARGUMENT_FIXED_TYPE_SUPER_NAME_NO_INVOCATION, AML_ARGUMENT_FIXED_TYPE_TARGET,      AML_ARGUMENT_FIXED_TYPE_TERM_ARG,
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_NAME_STRING, AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_TERM_ARG,    AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,              AML_ARGUMENT_FIXED_TYPE_SUPER_NAME,  AML_ARGUMENT_FIXED_TYPE_SUPER_NAME,      
     AML_ARGUMENT_FIXED_TYPE_WORD_DATA,    AML_ARGUMENT_FIXED_TYPE_SUPER_NAME,               AML_ARGUMENT_FIXED_TYPE_SUPER_NAME,  AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_TARGET,                   AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,   AML_ARGUMENT_FIXED_TYPE_DWORD_DATA,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,              AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,   AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,              AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,   AML_ARGUMENT_FIXED_TYPE_WORD_DATA,      
     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,  AML_ARGUMENT_FIXED_TYPE_NAME_STRING,              AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,   AML_ARGUMENT_FIXED_TYPE_NAME_STRING,      
     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,  AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,   AML_ARGUMENT_FIXED_TYPE_NAME_STRING,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_TERM_ARG,    AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_SUPER_NAME,   AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_TERM_ARG,    AML_ARGUMENT_FIXED_TYPE_TARGET,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_TARGET,      AML_ARGUMENT_FIXED_TYPE_TARGET,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,                AML_ARGUMENT_FIXED_TYPE_TERM_ARG,    AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,      
     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,     AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_TERM_ARG,    AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_NAME_STRING,  AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_TERM_ARG,    AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_SIMPLE_NAME,  AML_ARGUMENT_FIXED_TYPE_TERM_ARG,                 AML_ARGUMENT_FIXED_TYPE_TERM_ARG,    AML_ARGUMENT_FIXED_TYPE_TERM_ARG,      
     AML_ARGUMENT_FIXED_TYPE_TARGET,       AML_ARGUMENT_FIXED_TYPE_NAME_STRING,              AML_ARGUMENT_FIXED_TYPE_BYTE_DATA,   AML_ARGUMENT_FIXED_TYPE_DWORD_DATA,
     AML_ARGUMENT_FIXED_TYPE_BYTE_DATA
};

static const UINT8 AmlOpcodeVariableArgumentListSharedValueTable[] = {
    AML_ARGUMENT_VARIABLE_TYPE_TERM_LIST,
    AML_ARGUMENT_VARIABLE_TYPE_BYTE_LIST,
    AML_ARGUMENT_VARIABLE_TYPE_PACKAGE,
    AML_ARGUMENT_VARIABLE_TYPE_TERM_LIST,
    AML_ARGUMENT_VARIABLE_TYPE_FIELD_LIST,
};

static const AML_OPCODE_TABLE_ENTRY AmlOpcodeL2Table_0x5B[ 0xFF + 1 ] = {
    [0x01] = { 1, 0, 0x01, AML_ENCODING_GROUP_TERM_OBJECT,  0, 12, 2, 0, 0, },
    [0x02] = { 1, 0, 0x02, AML_ENCODING_GROUP_TERM_OBJECT,  0, 10, 1, 0, 0, },
    [0x12] = { 1, 1, 0x12, AML_ENCODING_GROUP_TERM_OBJECT,  0, 21, 2, 0, 0, },
    [0x13] = { 1, 0, 0x13, AML_ENCODING_GROUP_TERM_OBJECT,  0, 23, 4, 0, 0, },
    [0x1F] = { 1, 1, 0x1F, AML_ENCODING_GROUP_TERM_OBJECT,  0, 27, 6, 0, 0, },
    [0x20] = { 1, 1, 0x20, AML_ENCODING_GROUP_TERM_OBJECT,  0, 33, 2, 0, 0, },
    [0x21] = { 1, 0, 0x21, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 0, 0, },
    [0x22] = { 1, 0, 0x22, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 0, 0, },
    [0x23] = { 1, 1, 0x23, AML_ENCODING_GROUP_TERM_OBJECT,  0, 35, 2, 0, 0, },
    [0x24] = { 1, 0, 0x24, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x25] = { 1, 1, 0x25, AML_ENCODING_GROUP_TERM_OBJECT,  0, 38, 2, 0, 0, },
    [0x26] = { 1, 0, 0x26, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x27] = { 1, 0, 0x27, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x28] = { 1, 1, 0x28, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x29] = { 1, 1, 0x29, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x30] = { 1, 0, 0x30, AML_ENCODING_GROUP_DATA_OBJECT,  0, 0,  0, 0, 0, },
    [0x31] = { 1, 0, 0x31, AML_ENCODING_GROUP_DEBUG_OBJECT, 0, 0,  0, 0, 0, },
    [0x32] = { 1, 0, 0x32, AML_ENCODING_GROUP_TERM_OBJECT,  0, 42, 3, 0, 0, },
    [0x33] = { 1, 1, 0x33, AML_ENCODING_GROUP_TERM_OBJECT,  0, 0,  0, 0, 0, },
    [0x80] = { 1, 0, 0x80, AML_ENCODING_GROUP_TERM_OBJECT,  0, 45, 4, 0, 0, },
    [0x81] = { 1, 0, 0x81, AML_ENCODING_GROUP_TERM_OBJECT,  0, 12, 2, 4, 1, },
    [0x82] = { 1, 0, 0x82, AML_ENCODING_GROUP_TERM_OBJECT,  0, 10, 1, 0, 1, },
    [0x83] = { 1, 0, 0x83, AML_ENCODING_GROUP_TERM_OBJECT,  0, 89, 4, 0, 1, },
    [0x84] = { 1, 0, 0x84, AML_ENCODING_GROUP_TERM_OBJECT,  0, 49, 3, 0, 1, },
    [0x85] = { 1, 0, 0x85, AML_ENCODING_GROUP_TERM_OBJECT,  0, 10, 1, 0, 1, },
    [0x86] = { 1, 0, 0x86, AML_ENCODING_GROUP_TERM_OBJECT,  0, 52, 3, 4, 1, },
    [0x87] = { 1, 0, 0x87, AML_ENCODING_GROUP_TERM_OBJECT,  0, 55, 4, 4, 1, },
    [0x88] = { 1, 0, 0x88, AML_ENCODING_GROUP_TERM_OBJECT,  0, 59, 4, 0, 0, },
};

static const AML_OPCODE_TABLE_ENTRY AmlOpcodeL2Table_0x92[ 0xFF + 1 ] = {
    [0x93] = { 1, 1, 0x93, AML_ENCODING_GROUP_TERM_OBJECT, 0, 81, 2, 0, 0, },
    [0x94] = { 1, 1, 0x94, AML_ENCODING_GROUP_TERM_OBJECT, 0, 81, 2, 0, 0, },
    [0x95] = { 1, 1, 0x95, AML_ENCODING_GROUP_TERM_OBJECT, 0, 81, 2, 0, 0, },
};

static const AML_OPCODE_SUBTABLE_LIST_ENTRY AmlOpcodeL2SubTableList[] = {
    [0] = { .IsValid = 0 },
    [1] = { .IsValid = 1, .IsOptionalMatch = 0, .Entries = AmlOpcodeL2Table_0x5B },
    [2] = { .IsValid = 1, .IsOptionalMatch = 1, .Entries = AmlOpcodeL2Table_0x92 },
};

static const AML_OPCODE_TABLE_ENTRY AmlOpcodeL1Table[ 0xFF + 1 ] = {
    [0x00] = { 1, 0, 0x00, AML_ENCODING_GROUP_DATA_OBJECT,  0, 0,  0, 0, 0, },
    [0x01] = { 1, 0, 0x01, AML_ENCODING_GROUP_DATA_OBJECT,  0, 0,  0, 0, 0, },
    [0x06] = { 1, 0, 0x06, AML_ENCODING_GROUP_TERM_OBJECT,  0, 0,  2, 0, 0, },
    [0x08] = { 1, 0, 0x08, AML_ENCODING_GROUP_TERM_OBJECT,  0, 2,  2, 0, 0, },
    [0x0A] = { 1, 0, 0x0A, AML_ENCODING_GROUP_DATA_OBJECT,  0, 4,  1, 0, 0, },
    [0x0B] = { 1, 0, 0x0B, AML_ENCODING_GROUP_DATA_OBJECT,  0, 5,  1, 0, 0, },
    [0x0C] = { 1, 0, 0x0C, AML_ENCODING_GROUP_DATA_OBJECT,  0, 6,  1, 0, 0, },
    [0x0D] = { 1, 0, 0x0D, AML_ENCODING_GROUP_DATA_OBJECT,  0, 7,  2, 0, 0, },
    [0x0E] = { 1, 0, 0x0E, AML_ENCODING_GROUP_DATA_OBJECT,  0, 9,  1, 0, 0, },
    [0x10] = { 1, 0, 0x10, AML_ENCODING_GROUP_TERM_OBJECT,  0, 10, 1, 0, 1, },
    [0x11] = { 1, 1, 0x11, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 1, 1, },
    [0x12] = { 1, 1, 0x12, AML_ENCODING_GROUP_TERM_OBJECT,  0, 4,  1, 2, 2, },
    [0x13] = { 1, 1, 0x13, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 2, 2, },
    [0x14] = { 1, 0, 0x14, AML_ENCODING_GROUP_TERM_OBJECT,  0, 12, 2, 0, 1, },
    [0x15] = { 1, 0, 0x15, AML_ENCODING_GROUP_NAME_OBJECT,  0, 14, 3, 0, 0, },
    [0x2E] = { 1, 0, 0x2E, AML_ENCODING_GROUP_NAME_OBJECT,  0, 17, 2, 0, 0, },
    [0x2F] = { 1, 0, 0x2F, AML_ENCODING_GROUP_NAME_OBJECT,  0, 19, 2, 0, 0, },
    [0x30] = { 1, 0, 0x30, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x31] = { 1, 0, 0x31, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x32] = { 1, 0, 0x32, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x33] = { 1, 0, 0x33, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x34] = { 1, 0, 0x34, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x35] = { 1, 0, 0x35, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x36] = { 1, 0, 0x36, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x37] = { 1, 0, 0x37, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x38] = { 1, 0, 0x38, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x39] = { 1, 0, 0x39, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x41] = { 1, 0, 0x41, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x42] = { 1, 0, 0x42, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x43] = { 1, 0, 0x43, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x44] = { 1, 0, 0x44, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x45] = { 1, 0, 0x45, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x46] = { 1, 0, 0x46, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x47] = { 1, 0, 0x47, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x48] = { 1, 0, 0x48, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x49] = { 1, 0, 0x49, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x4a] = { 1, 0, 0x4a, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x4b] = { 1, 0, 0x4b, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x4c] = { 1, 0, 0x4c, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x4d] = { 1, 0, 0x4d, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x4e] = { 1, 0, 0x4e, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x4f] = { 1, 0, 0x4f, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x50] = { 1, 0, 0x50, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x51] = { 1, 0, 0x51, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x52] = { 1, 0, 0x52, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x53] = { 1, 0, 0x53, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x54] = { 1, 0, 0x54, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x55] = { 1, 0, 0x55, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x56] = { 1, 0, 0x56, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x57] = { 1, 0, 0x57, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x58] = { 1, 0, 0x58, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x59] = { 1, 0, 0x59, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x5B] = { 1, 0, 0x5B, AML_ENCODING_GROUP_NONE,         1, 4,  1, 0, 0, },
    [0x5C] = { 1, 0, 0x5C, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x5E] = { 1, 0, 0x5E, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x5F] = { 1, 0, 0x5F, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x5a] = { 1, 0, 0x5a, AML_ENCODING_GROUP_NAME_OBJECT,  0, 0,  0, 0, 0, },
    [0x60] = { 1, 0, 0x60, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x61] = { 1, 0, 0x61, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x62] = { 1, 0, 0x62, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x63] = { 1, 0, 0x63, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x64] = { 1, 0, 0x64, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x65] = { 1, 0, 0x65, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x66] = { 1, 0, 0x66, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x67] = { 1, 0, 0x67, AML_ENCODING_GROUP_LOCAL_OBJECT, 0, 0,  0, 0, 0, },
    [0x68] = { 1, 0, 0x68, AML_ENCODING_GROUP_ARG_OBJECT,   0, 0,  0, 0, 0, },
    [0x69] = { 1, 0, 0x69, AML_ENCODING_GROUP_ARG_OBJECT,   0, 0,  0, 0, 0, },
    [0x6A] = { 1, 0, 0x6A, AML_ENCODING_GROUP_ARG_OBJECT,   0, 0,  0, 0, 0, },
    [0x6B] = { 1, 0, 0x6B, AML_ENCODING_GROUP_ARG_OBJECT,   0, 0,  0, 0, 0, },
    [0x6C] = { 1, 0, 0x6C, AML_ENCODING_GROUP_ARG_OBJECT,   0, 0,  0, 0, 0, },
    [0x6D] = { 1, 0, 0x6D, AML_ENCODING_GROUP_ARG_OBJECT,   0, 0,  0, 0, 0, },
    [0x6E] = { 1, 0, 0x6E, AML_ENCODING_GROUP_ARG_OBJECT,   0, 0,  0, 0, 0, },
    [0x70] = { 1, 1, 0x70, AML_ENCODING_GROUP_TERM_OBJECT,  0, 63, 2, 0, 0, },
    [0x71] = { 1, 1, 0x71, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x72] = { 1, 1, 0x72, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x73] = { 1, 1, 0x73, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x74] = { 1, 1, 0x74, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x75] = { 1, 1, 0x75, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x76] = { 1, 1, 0x76, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x77] = { 1, 1, 0x77, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x78] = { 1, 1, 0x78, AML_ENCODING_GROUP_TERM_OBJECT,  0, 68, 4, 0, 0, },
    [0x79] = { 1, 1, 0x79, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x7A] = { 1, 1, 0x7A, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x7B] = { 1, 1, 0x7B, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x7C] = { 1, 1, 0x7C, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x7D] = { 1, 1, 0x7D, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x7E] = { 1, 1, 0x7E, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x7F] = { 1, 1, 0x7F, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x80] = { 1, 1, 0x80, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x81] = { 1, 1, 0x81, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x82] = { 1, 1, 0x82, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x83] = { 1, 1, 0x83, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 0, 0, },
    [0x84] = { 1, 1, 0x84, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x85] = { 1, 1, 0x85, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x86] = { 1, 0, 0x86, AML_ENCODING_GROUP_TERM_OBJECT,  0, 38, 2, 0, 0, },
    [0x87] = { 1, 1, 0x87, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x88] = { 1, 1, 0x88, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x89] = { 1, 1, 0x89, AML_ENCODING_GROUP_TERM_OBJECT,  0, 72, 6, 0, 0, },
    [0x8A] = { 1, 0, 0x8A, AML_ENCODING_GROUP_TERM_OBJECT,  0, 78, 3, 0, 0, },
    [0x8B] = { 1, 0, 0x8B, AML_ENCODING_GROUP_TERM_OBJECT,  0, 78, 3, 0, 0, },
    [0x8C] = { 1, 0, 0x8C, AML_ENCODING_GROUP_TERM_OBJECT,  0, 78, 3, 0, 0, },
    [0x8D] = { 1, 0, 0x8D, AML_ENCODING_GROUP_TERM_OBJECT,  0, 78, 3, 0, 0, },
    [0x8E] = { 1, 1, 0x8E, AML_ENCODING_GROUP_TERM_OBJECT,  0, 37, 1, 0, 0, },
    [0x8F] = { 1, 0, 0x8F, AML_ENCODING_GROUP_TERM_OBJECT,  0, 78, 3, 0, 0, },
    [0x90] = { 1, 1, 0x90, AML_ENCODING_GROUP_TERM_OBJECT,  0, 81, 2, 0, 0, },
    [0x91] = { 1, 1, 0x91, AML_ENCODING_GROUP_TERM_OBJECT,  0, 81, 2, 0, 0, },
    [0x92] = { 1, 1, 0x92, AML_ENCODING_GROUP_TERM_OBJECT,  2, 11, 1, 0, 0, },
    [0x93] = { 1, 1, 0x93, AML_ENCODING_GROUP_TERM_OBJECT,  0, 81, 2, 0, 0, },
    [0x94] = { 1, 1, 0x94, AML_ENCODING_GROUP_TERM_OBJECT,  0, 81, 2, 0, 0, },
    [0x95] = { 1, 1, 0x95, AML_ENCODING_GROUP_TERM_OBJECT,  0, 81, 2, 0, 0, },
    [0x96] = { 1, 1, 0x96, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x97] = { 1, 1, 0x97, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x98] = { 1, 1, 0x98, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x99] = { 1, 1, 0x99, AML_ENCODING_GROUP_TERM_OBJECT,  0, 40, 2, 0, 0, },
    [0x9C] = { 1, 1, 0x9C, AML_ENCODING_GROUP_TERM_OBJECT,  0, 65, 3, 0, 0, },
    [0x9D] = { 1, 1, 0x9D, AML_ENCODING_GROUP_TERM_OBJECT,  0, 83, 2, 0, 0, },
    [0x9E] = { 1, 1, 0x9E, AML_ENCODING_GROUP_TERM_OBJECT,  0, 85, 4, 0, 0, },
    [0x9F] = { 1, 0, 0x9F, AML_ENCODING_GROUP_TERM_OBJECT,  0, 0,  0, 0, 0, },
    [0xA0] = { 1, 0, 0xA0, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 0, 1, },
    [0xA1] = { 1, 0, 0xA1, AML_ENCODING_GROUP_TERM_OBJECT,  0, 0,  0, 0, 1, },
    [0xA2] = { 1, 0, 0xA2, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 0, 1, },
    [0xA3] = { 1, 0, 0xA3, AML_ENCODING_GROUP_TERM_OBJECT,  0, 0,  0, 0, 0, },
    [0xA4] = { 1, 0, 0xA4, AML_ENCODING_GROUP_TERM_OBJECT,  0, 11, 1, 0, 0, },
    [0xA5] = { 1, 0, 0xA5, AML_ENCODING_GROUP_TERM_OBJECT,  0, 0,  0, 0, 0, },
    [0xCC] = { 1, 0, 0xCC, AML_ENCODING_GROUP_TERM_OBJECT,  0, 0,  0, 0, 0, },
    [0xFF] = { 1, 0, 0xFF, AML_ENCODING_GROUP_DATA_OBJECT,  0, 0,  0, 0, 0, },
};