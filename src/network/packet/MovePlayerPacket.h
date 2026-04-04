#ifndef NET_MINECRAFT_NETWORK_PACKET__MovePlayerPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__MovePlayerPacket_H__

#include "../Packet.h"

class MovePlayerPacket : public Packet
{
public:
    int entityId;
    double x, y, z;
    double xd, yd, zd;   // 速度（新增）
    float xRot, yRot;

    MovePlayerPacket() {}
    MovePlayerPacket(int entityId, double x, double y, double z,
                     double xd, double yd, double zd,
                     float yRot, float xRot)
        : entityId(entityId), x(x), y(y), z(z),
          xd(xd), yd(yd), zd(zd),
          yRot(yRot), xRot(xRot) {}

    void write(RakNet::BitStream* bitStream) {
        bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_MOVEPLAYER));
        bitStream->Write(entityId);
        bitStream->Write(x); bitStream->Write(y); bitStream->Write(z);
        bitStream->Write(xd); bitStream->Write(yd); bitStream->Write(zd);
        bitStream->Write(yRot); bitStream->Write(xRot);
    }

    void read(RakNet::BitStream* bitStream) {
        bitStream->Read(entityId);
        bitStream->Read(x); bitStream->Read(y); bitStream->Read(z);
        bitStream->Read(xd); bitStream->Read(yd); bitStream->Read(zd);
        bitStream->Read(yRot); bitStream->Read(xRot);
    }

    void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
        callback->handle(source, this);
    }
};

#endif
