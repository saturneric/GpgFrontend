# Introduce Public Service

Except KeyServer, some advanced functions of GpgFrontend will use the services provided by GpgFrontendServer.

## GpgFrontend Server

[GpgFrontend Server](https://github.com/saturneric/GpgFrontendServer) is a server that provides some advanced services for GpgFrontend.

The project will develop with the development of GpgFrontend. 
The open source of the project means that the logic of the advanced services used by GpgFrontend is transparent. 
Users can deploy the server by themselves to enjoy some advanced functions provided by GpgFrontend. Or use the public server deployed on gpgfrontend.pub.

## Public Service

The GpgFrontend Server server will be deployed in a public server and will provide services for all valid clients by default. 
If you don't want to use public services, you can deploy GpgFrontend Server in a private server.

### Service Address

service.gpgfrontend.pub

### Cloud Service Provider

Tencent Cloud

### Cloud database service provider

TencentDB MySQL 8.0

### Port

The public server only uses port 9049 as the communication port.

## Security

The communication between GpgFrontend and public servers uses HTTPS protocol.
In addition, some security measures provided by GnuPG are used in the communication protocol of the application layer.
