using System.Buffers.Binary;
using System.IO;
using ServerListEditor.Encoding;
using ServerListEditor.Models;

namespace ServerListEditor.Bmd;

internal static class ServerListReader
{
    public static ServerListDocument Read(string path)
    {
        var bytes = File.ReadAllBytes(path);
        var groups = new List<ServerGroup>();
        var offset = 0;

        while (offset < bytes.Length)
        {
            EnsureRemaining(bytes, offset, ServerGroup.HeaderLength, "记录头");
            var header = bytes.AsSpan(offset, ServerGroup.HeaderLength).ToArray();
            ApplyBuxConvert(header);
            var descriptionLength = BinaryPrimitives.ReadInt16LittleEndian(header.AsSpan(51, 2));
            if (descriptionLength < 0)
                throw new InvalidDataException($"记录 {groups.Count} 的描述长度无效：{descriptionLength}。");

            var descriptionOffset = offset + ServerGroup.HeaderLength;
            EnsureRemaining(bytes, descriptionOffset, descriptionLength, "描述数据");
            var group = new ServerGroup
            {
                Header = header,
                Description = bytes.AsSpan(descriptionOffset, descriptionLength).ToArray(),
                Id = BinaryPrimitives.ReadUInt16LittleEndian(header.AsSpan(0, 2)),
                Name = MuEncoding.Decode(header.AsSpan(ServerGroup.NameOffset, ServerGroup.NameLength))
            };
            header.AsSpan(ServerGroup.ServerTypesOffset, ServerGroup.ServerTypesLength)
                .CopyTo(group.ServerTypes);
            groups.Add(group);
            offset = descriptionOffset + descriptionLength;
        }

        return new ServerListDocument { Groups = groups };
    }

    private static void EnsureRemaining(byte[] bytes, int offset, int length, string field)
    {
        if (length < 0 || offset > bytes.Length - length)
            throw new InvalidDataException($"ServerList.bmd 的 {field} 超出文件范围。");
    }

    internal static void ApplyBuxConvert(Span<byte> bytes)
    {
        ReadOnlySpan<byte> key = [0xFC, 0xCF, 0xAB];
        for (var i = 0; i < bytes.Length; i++)
            bytes[i] ^= key[i % key.Length];
    }
}
