using System.IO;
using System.Text;

namespace ServerListEditor.Encoding;

public static class MuEncoding
{
    private static readonly System.Text.Encoding Cp936 = CreateCp936();

    public static string Decode(ReadOnlySpan<byte> bytes)
    {
        var length = bytes.IndexOf((byte)0);
        return Cp936.GetString(length >= 0 ? bytes[..length] : bytes);
    }

    public static byte[] EncodeName(string value, int fieldLength)
    {
        var encoded = Cp936.GetBytes(value);
        if (encoded.Length >= fieldLength)
            throw new InvalidDataException($"服务器名称编码后必须少于 {fieldLength} 字节。");

        var result = new byte[fieldLength];
        encoded.CopyTo(result, 0);
        return result;
    }

    private static System.Text.Encoding CreateCp936()
    {
        System.Text.Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
        return System.Text.Encoding.GetEncoding(
            936,
            EncoderFallback.ExceptionFallback,
            DecoderFallback.ExceptionFallback);
    }
}
