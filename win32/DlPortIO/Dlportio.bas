Attribute VB_Name = "DLPortIO"
'****************************************************************************
'*  @doc INTERNAL
'*  @module dlportio.bas |
'*
'*  DriverLINX Port I/O Driver Interface
'*  <cp> Copyright 1996-1999 Scientific Software Tools, Inc.<nl>
'*  All Rights Reserved.<nl>
'*  DriverLINX is a registered trademark of Scientific Software Tools, Inc.
'*
'*  Win32 Prototypes for DriverLINX Port I/O
'*
'*  @comm
'*  Author: RoyF<nl>
'*  Date:   09/26/96 14:08:58
'*
'*  @group Revision History
'*  @comm
'*  $Revision: 2 $
'*  <nl>
'*  $Log: /DLPortIO/API/DLPORTIO.BAS $
' 
' 2     3/03/99 5:25p Kevind
' Removed any reference for customer to call us when encountering bugs,
' also removed our old address info.
'
' 1     9/27/96 2:03p Royf
' Initial revision.
'*
'****************************************************************************


Public Declare Function DlPortReadPortUchar Lib "dlportio.dll" (ByVal Port As Long) As Byte
Public Declare Function DlPortReadPortUshort Lib "dlportio.dll" (ByVal Port As Long) As Integer
Public Declare Function DlPortReadPortUlong Lib "dlportio.dll" (ByVal Port As Long) As Long

Public Declare Sub DlPortReadPortBufferUchar Lib "dlportio.dll" (ByVal Port As Long, Buffer As Any, ByVal Count As Long)
Public Declare Sub DlPortReadPortBufferUshort Lib "dlportio.dll" (ByVal Port As Long, Buffer As Any, ByVal Count As Long)
Public Declare Sub DlPortReadPortBufferUlong Lib "dlportio.dll" (ByVal Port As Long, Buffer As Any, ByVal Count As Long)

Public Declare Sub DlPortWritePortUchar Lib "dlportio.dll" (ByVal Port As Long, ByVal Value As Byte)
Public Declare Sub DlPortWritePortUshort Lib "dlportio.dll" (ByVal Port As Long, ByVal Value As Integer)
Public Declare Sub DlPortWritePortUlong Lib "dlportio.dll" (ByVal Port As Long, ByVal Value As Long)

Public Declare Sub DlPortWritePortBufferUchar Lib "dlportio.dll" (ByVal Port As Long, Buffer As Any, ByVal Count As Long)
Public Declare Sub DlPortWritePortBufferUshort Lib "dlportio.dll" (ByVal Port As Long, Buffer As Any, ByVal Count As Long)
Public Declare Sub DlPortWritePortBufferUlong Lib "dlportio.dll" (ByVal Port As Long, Buffer As Any, ByVal Count As Long)

