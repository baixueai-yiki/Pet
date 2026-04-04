from pathlib import Path
path = Path('Source/Systems/Chat/Chat.cpp')
text = path.read_text()
start = text.find('static LRESULT CALLBACK TalkProc')
if start != -1:
    end = text.find('// ¶Ô»°Êä³ö', start)
    if end != -1:
        text = text[:start] + text[end:]
        path.write_text(text)
