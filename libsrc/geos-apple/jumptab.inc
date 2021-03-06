;
;Apple GEOS System Jump Table
;Source: The Hitchhiker's Guide To GEOS
;http://lyonlabs.org/commodore/onrequest/geos-manuals/The_Hitchhikers_Guide_to_GEOS.pdf

ReadClock		=	$0800
ClockInt		=	$0803
SetTimeDate		=	$0806
SetAlarm		=	$0809
ResetAlarm		=	$080c
InitForPrint		=	$6000
StartPrint		=	$6003
StartASCII		=	$6006
PrintBuffer		=	$6009
PrintASCII		=	$600c
StopPrint		=	$600f
GetMode			=	$6012
SetMode			=	$6015
CancelPrint		=	$6018
InitCard		=	$6700
OpenCard		=	$6703
OutputByte		=	$6706
InputByte		=	$6709
CloseCard		=	$670c
InfoCard		=	$670f
StatusCard		=	$6712
ClearCard		=	$6715
AuxDMain		=	$efa0
AuxDInt			=	$efa3
AuxDKeyFilter		=	$efa6
AuxDExit		=	$efa9
InitMouse		=	$f000
SlowMouse		=	$f003
UpdateMouse		=	$f006
KeyFilter		=	$f009
MainLoop		=	$fe00
InterruptMain		=	$fe03
StartAppl		=	$fe06
DoInlineReturn		=	$fe09
DoIcons			=	$fe0c
DoMenu			=	$fe0f
ReDoMenu		=	$fe12
DoPreviousMenu		=	$fe15
GotoFirstMenu		=	$fe18
RecoverMenu		=	$fe1b
RecoverAllMenus		=	$fe1e
DrawPoint		=	$fe21
TestPoint		=	$fe24
HorizontalLine		=	$fe27
VerticalLine		=	$fe2a
InvertLine		=	$fe2d
RecoverLine		=	$fe30
DrawLine		=	$fe33
SetPattern		=	$fe36
Rectangle		=	$fe39
i_Rectangle		=	$fe3c
FrameRectangle		=	$fe3f
i_FrameRectangle	=	$fe42
InvertRectangle		=	$fe45
RecoverRectangle	=	$fe48
i_RecoverRectangle	=	$fe4b
ImprintRectangle	=	$fe4e
i_ImprintRectangle	=	$fe51
BitmapUp		=	$fe54
i_BitmapUp		=	$fe57
BitmapClip		=	$fe5a
BitOtherClip		=	$fe5d
GraphicsString		=	$fe60
i_GraphicsString	=	$fe63
GetScanLine		=	$fe66
PutString		=	$fe69
i_PutString		=	$fe6c
PutDecimal		=	$fe6f
GetString		=	$fe72
GetNextChar		=	$fe75
InitTextPrompt		=	$fe78
PromptOn		=	$fe7b
PromptOff		=	$fe7e
PutChar			=	$fe81
GetRealSize		=	$fe84
GetCharWidth		=	$fe87
LoadCharSet		=	$fe8a
UseSystemFont		=	$fe8d
SmallPutChar		=	$fe90
StartMouseMode		=	$fe93
MouseOff		=	$fe96
MouseUp			=	$fe99
IsMseInRegion		=	$fe9c
ClearMouseMode		=	$fe9f
DrawSprite		=	$fea2
PosSprite		=	$fea5
EnablSprite		=	$fea8
DisablSprite		=	$feab
InitProcesses		=	$feae
RestartProcess		=	$feb1
BlockProcess		=	$feb4
UnblockProcess		=	$feb7
FreezeProcess		=	$feba
UnfreezeProcess		=	$febd
Sleep			=	$fec0
EnableProcess		=	$fec3
DShiftLeft		=	$fec6
DShiftRight		=	$fec9
BBMult			=	$fecc
BMult			=	$fecf
DMult			=	$fed2
Ddiv			=	$fed5
DSdiv			=	$fed8
Dabs			=	$fedb
Dnegate			=	$fede
Ddec			=	$fee1
GetRandom		=	$fee4
CopyString		=	$fee7
CopyFString		=	$feea
CmpString		=	$feed
CmpFString		=	$fef0
Panic			=	$fef3
MoveData		=	$fef6
i_MoveData		=	$fef9
ClearRam		=	$fefc
FillRam			=	$feff
i_FillRam		=	$ff02
InitRam			=	$ff05
CallRoutine		=	$ff08
NewBitOtherClip		=	$ff0e
FirstInit		=	$ff11
CRC			=	$ff14
DoDlgBox		=	$ff17
RstrFrmDialogue		=	$ff1a
NewBitUp		=	$ff1d
IrqMiddle		=	$ff20
TempHideMouse		=	$ff29
SoftSprHandler		=	$ff2c
SetMsePic		=	$ff2f
InitSprite		=	$ff32
SaveFontData		=	$ff35
RestoreFontData		=	$ff38
EraseCharacter		=	$ff3e
ReadByte		=	$ff41
GetLdVars		=	$ff44
SetLdVars		=	$ff47
InitForDialog		=	$ff4a
WarmStart		=	$ff4d
RecoverSysRam		=	$ff53
GetPtrCurDkNm		=	$ff56
EnterDeskTop		=	$ff59
CopyScreenBlock		=	$ff5c
CopyLine		=	$ff5f
GetScreenLine		=	$ff62
PutScreenLine		=	$ff65
DivideBySeven		=	$ff68
MoveAuxData		=	$ff6b
GetSpriteData		=	$ff6e
SwapMainAndAux		=	$ff71
ReadBackLine		=	$ff74
LoadAuxSet		=	$ff77
HideOnlyMouse		=	$ff7a
GetPattern		=	$ff7d
SetUserPattern		=	$ff80
Bell			=	$ff86
ImprintLine		=	$ff8f
CopyFullScreen		=	$ff92
NewBitClip		=	$ff95
ReadScanLine		=	$ff98
i_NewBitUp		=	$ff9e
