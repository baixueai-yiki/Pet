from pathlib import Path
path = Path('Source/Systems/Chat/Chat.cpp')
data = path.read_text(encoding='utf-8')
start = data.find('// 显示文字输入窗口')
end = data.find('void ChatRecordInteraction()', start)
if start == -1 or end == -1:
    raise SystemExit('not found')
end_block = data.find('}', end)
end_block = end_block + 1
new_chunk = '''
// 显示文字输入窗口
void ChatShowInput(HWND hwndParent)
{
    ChatPanels::ShowInput(hwndParent);
}

void ChatUpdateInputPosition() {}

void ChatUpdateButtonPosition() {}

void ChatUpdateTalkPosition() {}

void ChatUpdateTaskListPosition() {}

void ChatRecordInteraction()
{
    EnsureStateLoaded();
    s_state.lastInteraction = GetUnixTimeSeconds();
}
'''
new_data = data[:start] + new_chunk + data[end_block:]
path.write_text(new_data, encoding='utf-8')
