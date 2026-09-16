using System.IO;
using ServerListEditor.Encoding;
using ServerListEditor.Models;

namespace ServerListEditor.Bmd;

internal static class ServerListWriter
{
    public static void Write(ServerListDocument document, string path)
    {
        using var stream = new MemoryStream();
        foreach (var group in document.Groups)
        {
            var header = group.Header.ToArray();
            MuEncoding.EncodeName(group.Name, ServerGroup.NameLength)
                .CopyTo(header, ServerGroup.NameOffset);
            group.ServerTypes.CopyTo(header, ServerGroup.ServerTypesOffset);
            ServerListReader.ApplyBuxConvert(header);
            stream.Write(header);
            stream.Write(group.Description);
        }

        File.WriteAllBytes(path, stream.ToArray());
    }
}
