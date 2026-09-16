using ServerListEditor.Models;

namespace ServerListEditor.Bmd;

internal sealed class ServerListDocument
{
    public required List<ServerGroup> Groups { get; init; }

    public void AddGroup(ServerGroup group) => Groups.Add(group);

    public void RemoveGroup(int index) => Groups.RemoveAt(index);
}
