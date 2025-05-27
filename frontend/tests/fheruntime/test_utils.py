from typing import List

def load_data_array(file) -> List[bytes]:
    with open(file + '.num', 'r') as f:
        size = int(f.read())

    lst = []
    for i in range(size):
        with open(file + '.' + str(i), 'rb') as f:
            content = f.read()
            lst.append(content)
    return lst

def save_data_array(data, file):
    size = len(data)
    with open(file + '.num', 'w') as f:
        f.write(str(size))
    
    for i in range(size):
        with open(file + '.' + str(i), 'wb') as f:
            f.write(data[i])
