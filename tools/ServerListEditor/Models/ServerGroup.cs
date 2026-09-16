using System.Buffers.Binary;

namespace ServerListEditor.Models;

internal sealed class ServerGroup
{
    internal const int HeaderLength = 53;
    internal const int NameOffset = 2;
    internal const int NameLength = 32;
    internal const int ServerTypesOffset = 36;
    internal const int ServerTypesLength = 15;

    public ushort Id { get; internal init; }
    public string Name { get; set; } = string.Empty;
    public byte[] ServerTypes { get; } = new byte[ServerTypesLength];

    internal required byte[] Header { get; init; }
    internal required byte[] Description { get; init; }

    public static ServerGroup CreateNew(ushort id)
    {
        var header = new byte[HeaderLength];
        BinaryPrimitives.WriteUInt16LittleEndian(header.AsSpan(0, 2), id);
        return new ServerGroup
        {
            Id = id,
            Name = "新服务器",
            Header = header,
            Description = []
        };
    }
}
