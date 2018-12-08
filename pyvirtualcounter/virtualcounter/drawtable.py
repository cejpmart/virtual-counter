def lpad(str, width):
    return ' ' * (width - len(str)) + str

def rpad(str, width):
    return str + ' ' * (width - len(str))

class Table:
    def __init__(self, cols):
        self.cols = [dict(title=title, width=max(width, len(title)), format=format) for title, width, format in cols]

        print()

        header = ' | '.join([rpad(col['title'], col['width']) for col in self.cols])
        print(header)
        print('=' * len(header))

    def row(self, *args):
        row = ' | '.join([lpad(col['format'] % args[i], col['width']) for i, col in enumerate(self.cols)])
        print(row)
