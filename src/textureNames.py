import glob
########################### Texture Names ###########################
files = []
amounts = []
add = True

for file in glob.glob('./src/Textures/*.png'):
    temp1 = str(file).split('/')[-1]
    temp2 = temp1.split('.')[0]
    temp3 = temp2.split('_')[0]
    for i in range(len(files)):
        if(files[i] == temp3):
            add = False

    if(add):
        files.append(temp3)
    else:
        add = True

open("texture_names.txt", "w").close()
open("texture_amounts.txt", "w").close()

files.sort()

outStr = ""
enterCount = 0;

for i in range(0,len(files)):
    enterCount += 1
    outStr = outStr + '"' +files[i] + '_block"'
    if i != (len(files)-1):
        outStr += ", "
    if enterCount % 10 == 0:
        outStr += "\n"

file = open("texture_names.txt", "a")
file.write(outStr)
file.close()

########################### Texture Amounts ###########################

allFiles = []
allFilesFirstName = []
add = True
deleteOne = False
outStr = ""
enterCount = 0;
counter = 1
underScore = 0
for textureFile1 in glob.glob('./src/Textures/*.png'):
    temp1 = str(textureFile1).split('/')[-1]
    allFiles.append(temp1.split('.')[0])
    allFilesFirstName.append(temp1.split('_')[0])

allFilesFirstName.sort()
allFiles.sort()

for i in range(len(files)):
    for j in range(len(allFiles)):
        for k in range(len(allFiles[j])):
            if(allFiles[j][k] == "_"):
                underScore += 1
        if(underScore == 2 and allFilesFirstName[j] == files[i]):
            counter += 1
            deleteOne = True
            
        underScore = 0
    enterCount += 1
    if(deleteOne):
        outStr = outStr + str(counter-1)
    else:
        outStr = outStr + str(counter)
    if i != (len(files)-1):
        outStr += ", "
    if enterCount % 10 == 0:
        outStr += "\n"
    deleteOne = False
    counter = 1

file = open("texture_amounts.txt", "a")
file.write(outStr)
file.close()

########################### Texture Real Indexes ###########################

files = []
filesAll = []
indexes = []
add = True
for file in glob.glob('./src/Textures/*.png'):
    temp1 = str(file).split('/')[-1]
    temp2 = temp1.split('.')[0]
    temp3 = temp2.split('_')[0]
    for i in range(len(files)):
        if(files[i] == temp3):
            add = False

    if(add):
        files.append(temp3)
    else:
        add = True
    filesAll.append(temp3)

files.sort()
filesAll.sort()

for j in range(len(files)):
    first = True
    for k in range(len(filesAll)):
        if(first == True and files[j] == filesAll[k]):
            first = False
            indexes.append(k)

open("texture_real_index.txt", "w").close()

outStr = ""
enterCount = 0;

for i in range(0,len(indexes)):
    enterCount += 1
    outStr = outStr + str(indexes[i])
    if i != (len(indexes)-1):
        outStr += ", "
    if enterCount % 10 == 0:
        outStr += "\n"

file = open("texture_real_index.txt", "a")
file.write(outStr)
file.close()