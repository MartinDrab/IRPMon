Unit VSockConnector;

Interface

Const
  VNCI_VERSION_INVALID      = $FFFFFFFF;

Function VSockConn_LocalId:Cardinal; Cdecl;
Function VSockConn_VMCIVersion:Cardinal; Cdecl;

Implementation

Const
  LibraryName = 'vsock-connector.dll';

Function VSockConn_LocalId:Cardinal; Cdecl; External LibraryName;
Function VSockConn_VMCIVersion:Cardinal; Cdecl; External LibraryName;


End.
