with open("logo.txt", encoding="utf-8") as f:
    for line in f:
        line = line.rstrip("\n")
        print(f'    VGA::terminal.write("{line}\\n");')
