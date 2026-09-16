using Microsoft.Win32;
using ServerListEditor.Bmd;
using ServerListEditor.Encoding;
using ServerListEditor.Models;
using System.Collections.ObjectModel;
using System.IO;
using System.Windows;
using System.Windows.Controls;

namespace ServerListEditor;

public partial class MainWindow : Window
{
    private const string DefaultDirectory = @"D:\TAREN_MU_S6\Data\Local";
    private const int ServerTypeCount = 15;
    private readonly ObservableCollection<ServerGroup> groups = [];
    private readonly ObservableCollection<ServerLine> lines = [];
    private ServerListDocument? document;
    private string? currentPath;

    public MainWindow()
    {
        InitializeComponent();
        GroupGrid.ItemsSource = groups;
        LineGrid.ItemsSource = lines;
        ((DataGridComboBoxColumn)LineGrid.Columns[2]).ItemsSource = ServerTypeOption.All;
    }

    private void Open_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog
        {
            Filter = "ServerList.bmd|ServerList.bmd|BMD 文件|*.bmd|所有文件|*.*",
            InitialDirectory = Directory.Exists(DefaultDirectory) ? DefaultDirectory : string.Empty,
            FileName = "ServerList.bmd"
        };
        if (dialog.ShowDialog(this) == true) LoadFile(dialog.FileName);
    }

    private void Save_Click(object sender, RoutedEventArgs e)
    {
        if (document is not null && currentPath is not null) SaveFile(currentPath);
    }

    private void SaveAs_Click(object sender, RoutedEventArgs e)
    {
        if (document is null) return;
        var dialog = new SaveFileDialog { FileName = "ServerList.bmd", Filter = "ServerList.bmd|ServerList.bmd|BMD 文件|*.bmd" };
        if (dialog.ShowDialog(this) == true) SaveFile(dialog.FileName);
    }

    private void AddGroup_Click(object sender, RoutedEventArgs e)
    {
        if (document is null) return;
        var nextId = GetNextGroupId();
        if (nextId is null)
        {
            ShowError("新增失败", new InvalidOperationException("ServerGroup ID 已达到最大值 65535。"));
            return;
        }

        var group = ServerGroup.CreateNew(nextId.Value);
        document.AddGroup(group);
        groups.Add(group);
        GroupGrid.SelectedIndex = groups.Count - 1;
        GroupGrid.ScrollIntoView(GroupGrid.SelectedItem);
        UpdateStatus();
    }

    private void DeleteGroup_Click(object sender, RoutedEventArgs e)
    {
        var index = GroupGrid.SelectedIndex;
        if (document is null || index < 0) return;

        const string message = "即将删除整个 ServerGroup。\n\n不会重新编号已有 ID，也不会单独删除 Slot。\n请确认服务端配置是否同步。\n\n请确认。";
        if (MessageBox.Show(this, message, "删除服务器组确认", MessageBoxButton.OKCancel, MessageBoxImage.Warning) != MessageBoxResult.OK)
            return;

        document.RemoveGroup(index);
        groups.RemoveAt(index);
        if (groups.Count > 0) GroupGrid.SelectedIndex = Math.Min(index, groups.Count - 1);
        else lines.Clear();
        UpdateStatus();
    }

    private void Group_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        lines.Clear();
        if (GroupGrid.SelectedItem is not ServerGroup group) return;
        for (var slot = 0; slot < ServerTypeCount; slot++)
            lines.Add(new ServerLine(slot, group.ServerTypes[slot]));
    }

    private void LoadFile(string path)
    {
        try
        {
            document = ServerListReader.Read(path);
            groups.Clear();
            foreach (var group in document.Groups) groups.Add(group);
            currentPath = path;
            UpdateStatus();
            if (groups.Count > 0) GroupGrid.SelectedIndex = 0;
        }
        catch (Exception ex) { ShowError("打开失败", ex); }
    }

    private void SaveFile(string path)
    {
        try
        {
            GroupGrid.CommitEdit(DataGridEditingUnit.Cell, true);
            GroupGrid.CommitEdit(DataGridEditingUnit.Row, true);
            LineGrid.CommitEdit(DataGridEditingUnit.Cell, true);
            LineGrid.CommitEdit(DataGridEditingUnit.Row, true);
            SyncLineTypes();
            ValidateGroups();
            ServerListWriter.Write(document!, path);
            currentPath = path;
            UpdateStatus();
            MessageBox.Show(this, "ServerList.bmd 已保存。", "完成", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex) { ShowError("保存失败", ex); }
    }

    private void SyncLineTypes()
    {
        if (GroupGrid.SelectedItem is not ServerGroup group) return;
        for (var slot = 0; slot < lines.Count; slot++) group.ServerTypes[slot] = lines[slot].Type;
    }

    private void ValidateGroups()
    {
        var ids = new HashSet<ushort>();
        foreach (var group in groups)
        {
            if (!ids.Add(group.Id))
                throw new InvalidDataException($"ServerGroup ID {group.Id} 重复，禁止保存。");
            if (string.IsNullOrWhiteSpace(group.Name))
                throw new InvalidDataException($"服务器组 ID {group.Id} 的名称不能为空。");
            _ = MuEncoding.EncodeName(group.Name, ServerGroup.NameLength);
            if (group.ServerTypes.Length != ServerTypeCount || group.ServerTypes.Any(type => type > 3))
                throw new InvalidDataException($"服务器组 ID {group.Id} 存在线路类型错误，必须为 0-3。");
        }
    }

    private ushort? GetNextGroupId()
    {
        if (groups.Count == 0) return 0;
        var maximum = groups.Max(group => group.Id);
        return maximum == ushort.MaxValue ? null : (ushort)(maximum + 1);
    }

    private void UpdateStatus()
    {
        PathText.Text = $"当前文件：{currentPath}";
        CountText.Text = $"服务器组数量：{groups.Count}";
    }

    private void ShowError(string title, Exception exception) =>
        MessageBox.Show(this, exception.Message, title, MessageBoxButton.OK, MessageBoxImage.Error);

    private sealed class ServerLine
    {
        public int Slot { get; }
        public int LineNumber => Slot + 1;
        public byte Type { get; set; }

        public ServerLine(int slot, byte type)
        {
            Slot = slot;
            Type = type;
        }
    }

    private sealed record ServerTypeOption(byte Value, string Label)
    {
        public static IReadOnlyList<ServerTypeOption> All { get; } =
        [
            new(0, "普通"),
            new(1, "Non-PVP"),
            new(2, "Gold PVP"),
            new(3, "Gold")
        ];
    }
}
