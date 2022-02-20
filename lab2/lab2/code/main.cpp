#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include<queue>

using namespace std;
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

#pragma pack(1) //设置对其方式，以1字节对齐
//引导区数据
struct BPB {
    char BS_OEMName[8];     //OEM字符串：mkfs.fat
    ushort BPB_BytsPerSEC;  //每扇区字节数：512
    byte BPB_SecPerClus;    //每簇占用的扇区数：1
    ushort BPB_RsvdSecCnt;  //boot占用的扇区数：1
    byte BPB_NumFATs;       //FAT表的记录数：2
    ushort BPB_RootEntCnt;  //最大根目录文件数：224
    ushort BPB_TotSec16;    //逻辑扇区总数：2880
    byte BPB_Media;         //媒体描述符：240
    ushort BPB_FATSz16;     //每个FAT占用的扇区数：9
    ushort BPB_SecPerTrk;   //每个磁道扇区数：18
    ushort BPB_NumHeads;    //磁头数：2
    uint BPB_Hiddsec;       //隐藏扇区数：0
    uint BPB_TotSec32;      // 如果BPB_TotSec16是0，则在这里记录：0
    byte BS_DrvNum;         // 中断13的驱动器号：0
    byte BS_Reserved1;      // 未使用：1
    byte BS_BootSig;        // 扩展引导标志：41
    uint BS_VolID;          // 卷序列号
    byte BS_VolLab[11];     // 卷标
    byte BS_FileSysType[8]; // 文件系统类型：FAT12
};

//根目录区目录项
struct RootEntry {
    char DIR_Name[11];  //文件名8字节，扩展名3字节
    byte DIR_Attr;      //文件属性
    byte reserve[10];   //保留位
    ushort DIR_WrtTime; //最后一次写入时间
    ushort DIR_WrtDate; //最后一次写入日期
    ushort DIR_FstClus; //文件开始的簇号
    uint DIR_FileSize;  //文件大小
};
#pragma pack()

vector<string> commandSplit(string &);

void ls(queue<string> &);

void lsl(queue<string>);

void lsOut(vector<pair<string, char> >);

void trim(string &);

vector<string> split(string, string);

extern "C" {
void sprint(const char *);
};

void solveInput();

RootEntry findCluster(string, int);

int calNextCluster(int);

void findSubDirectoryAndFile(queue<string> &, string path, RootEntry &);

string getName(RootEntry);

string getExtension(RootEntry);

void calNextLevel(RootEntry, bool, string);

void cat(string path);

void redPrint(const char *s) {
//    cout << s;
    sprint("\033[31m");
    sprint(s);
    sprint("\033[0m");
}

void whitePrint(const char *s) {
    sprint(s);
//    cout << s;
}


BPB bpb;
vector<RootEntry> rootEntries(224);
ifstream fin;
char *fat1;
vector<pair<string, char> > toPrint; //待打印的队列
vector<RootEntry> nextLevel;    //子文件夹和文件的目录项






void lsl(queue<string> directories) {
    while (!directories.empty()) {
        queue<string> q;
        string path = directories.front();
        directories.pop();
        if (path == "/") {
            //for循环结束后，q中应该保存根目录下的所有子目录，nextLevel中应该保存根目录下所有指向子目录和子文件的目录项
            for (int i = 0; i < rootEntries.size(); i++) {
                RootEntry current = rootEntries[i];
                findSubDirectoryAndFile(q, path, current);
            }

            //queue保存了子目录
            int subDirectories = q.size();
            int subFiles = nextLevel.size() - subDirectories;
            //cout << path << " " << subDirectories << " " << subFiles << ":" << endl;
            string s = path + " " + to_string(subDirectories) + " " + to_string(subFiles) + ":" + "\n";
            whitePrint(s.c_str());

            for (int i = 0; i < nextLevel.size(); i++) {     //对于根目录下的每一项
                calNextLevel(nextLevel[i], true, "/");
            }
        } else {
            vector<string> hierarchicalDirectories = split(path.substr(1, path.size() - 1), "/");
            int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //开始簇号是根目录区首簇
            RootEntry current;
            for (int i = 0; i < rootEntries.size(); i++) {        //先找到第一级目录，跳转到数据区再说
                string name = getName(rootEntries[i]);
                if (name == hierarchicalDirectories[0]) {
                    startCluster = rootEntries[i].DIR_FstClus;
                    current = rootEntries[i];
                    break;
                } else {
                    startCluster = -1;
                }
            }

            if (startCluster == -1) {
                throw "Wrong Path!\n";
            }

            for (int i = 1; i < hierarchicalDirectories.size(); i++) {
                current = findCluster(hierarchicalDirectories[i], startCluster);    //由上一级目录开始的簇来寻找当前目录开始的簇
                startCluster = current.DIR_FstClus;
                if (startCluster == -1 || startCluster >= 0xFF8) { //当前目录不存在
                    throw "Wrong Path!\n";
                }
            }
            //current现在保存了指向指定目录的目录项
            calNextLevel(current, false, path);     //输出总览的第一行

            //现在currentCluster保存待递归打印的目录的起始簇
            int currentCluster = startCluster;   //存一下
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
            while (true) {
                //每次寻找一个簇
                for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
                    //依次读取指定目录下的每一个目录项
                    fin.read((char *) &current, sizeof(current));
                    findSubDirectoryAndFile(q, path, current);
                }
                currentCluster = calNextCluster(currentCluster);
                if (currentCluster >= 0xFF8) {
                    break;
                }
            }
            for (int i = 0; i < nextLevel.size(); i++) {
                calNextLevel(nextLevel[i], true, path);
            }
        }
        //cout << endl;
        whitePrint("\n");
        nextLevel.clear();      //防止对递归产生干扰
        lsl(q);
        toPrint.clear();        //防止对其余命令产生干扰
    }
}

void calNextLevel(RootEntry temp, bool sub, string path) {    //path保存它的路径
    string directoryName = getName(temp);
    getName(temp);
    if (directoryName == "." || directoryName == "..") {
        //cout << directoryName << endl;
        string s = directoryName + "\n";
        redPrint(s.c_str());
        return;
    }

    int subDirectories = 0;
    int subFiles = 0;

    if (temp.DIR_Attr == 0x20) {    //当前为文件
        string name = getName(temp);
        string extension = getExtension(temp);
        //cout << name << "." << extension << "  " << temp.DIR_FileSize << endl;
        string s = name + "." + extension + "  " + to_string(temp.DIR_FileSize) + "\n";
        whitePrint(s.c_str());
    } else {  //当前为文件夹
        int currentCluster = temp.DIR_FstClus;
        fin.seekg((bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                  bpb.BPB_RootEntCnt * 32, ios::beg);
        while (true) {
            //每次寻找一个簇
            for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
                RootEntry current;
                fin.read((char *) &current, sizeof(current));
                if (current.DIR_Name[0] != '\xe5') {
                    if (current.DIR_Attr == 0x20) {   //文件
                        subFiles++;
                    } else if (current.DIR_Attr == 0x10) {    //文件夹
                        string name = getName(current);
                        if (name != "." && name != "..") {
                            subDirectories++;
                        }
                    }
                }
            }

            currentCluster = calNextCluster(currentCluster);
            if (currentCluster >= 0xFF8) {
                if (sub) {   //当前文件夹是作为子文件夹进行统计的
                    //cout << directoryName << " " << subDirectories << " " << subFiles << endl;
                    string s1 = directoryName + " ";
                    string s2 = to_string(subDirectories) + " " + to_string(subFiles) + "\n";
                    redPrint(s1.c_str());
                    whitePrint(s2.c_str());
                } else {      //当前文件夹是作为根文件夹统计的
                    //cout << path << "/" << " " << subDirectories << " " << subFiles << ":" << endl;
                    string s = path + "/" + " " + to_string(subDirectories) + " " + to_string(subFiles) + ":" "\n";
                    whitePrint(s.c_str());
                }
                break;
            }
        }
    }
}


void ls(queue<string> &directories) {
    while (!directories.empty()) {      //待访问目录不为空
        queue<string> q;
        string path = directories.front();  //取出队首元素，访问其下一层目录
        directories.pop();
        if (path == "/") {   //访问根目录的子目录
            //cout << "/" << ":" << endl;
            string s = "/:\n";
            whitePrint(s.c_str());
            for (int i = 0; i < rootEntries.size(); i++) {
                RootEntry &current = rootEntries[i];
                findSubDirectoryAndFile(q, path, current);
            }
        } else {
            //cout << path << "/" << ":" << endl;
            vector<string> hierarchicalDirectories = split(path.substr(1, path.size() - 1), "/");
            int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //开始簇号是根目录区首簇
            for (int i = 0; i < rootEntries.size(); i++) {        //先找到第一级目录，跳转到数据区再说
                string name = "";
                name = getName(rootEntries[i]);
                if (name == hierarchicalDirectories[0]) {
                    startCluster = rootEntries[i].DIR_FstClus;
                    break;
                } else{
                    startCluster = -1;
                }
            }

            if (startCluster == -1) {
                throw "Wrong Path!\n";
            }


            for (int i = 1; i < hierarchicalDirectories.size(); i++) {
                startCluster = findCluster(hierarchicalDirectories[i],
                                           startCluster).DIR_FstClus;    //由上一级目录开始的簇来寻找当前目录开始的簇
                if (startCluster == -1 || startCluster >= 0xFF8) { //当前目录不存在
                    throw "Wrong Path!\n";
                }
            }

            string s = path + "/:\n";
            whitePrint(s.c_str());

            //现在currentCluster保存待递归打印的目录的起始簇
            int currentCluster = startCluster;   //存一下
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
            while (true) {
                //每次寻找一个簇
                for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
                    RootEntry current;
                    fin.read((char *) &current, sizeof(current));
                    findSubDirectoryAndFile(q, path, current);
                }
                currentCluster = calNextCluster(currentCluster);
                if (currentCluster >= 0xFF8) {
                    break;
                }
            }
        }
        //在递归访问下一层次之前一定要先把当前层次的输出出来
        lsOut(toPrint);
        toPrint.clear();
        ls(q);
    }
}


//path：下一级目录
//cluster 上一级目录对应的数据区的保存目录的簇，是数据区那块的
RootEntry findCluster(string path, int startCluster) {
    RootEntry temp;
    int currentCluster = startCluster;   //存一下
    fin.seekg((bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
              bpb.BPB_RootEntCnt * 32, ios::beg);
    while (true) {
        //每次寻找一个簇
        for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
            fin.read((char *) &temp, sizeof(temp));
            string name = getName(temp);
            string extension = getExtension(temp);
            if (extension != "") {
                name += ".";
                name += extension;
            }

            if (name == path) {
                return temp;    //找到指定目录了，将其对应的数据区的簇返回
            }
        }
        currentCluster = calNextCluster(currentCluster);
        if (currentCluster >= 0xFF8) {
            temp.DIR_FstClus = -1;
            return temp;    //已经找完当前目录的所有子目录了，仍然没有找到对应路径，返回-1;
        } else {
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
        }
    }
}


int calNextCluster(int currentCluster) {
    int startByte = (currentCluster / 2) * 3;
    byte firstByte = fat1[startByte];
    byte secondByte = fat1[startByte + 1];
    byte thirdByte = fat1[startByte + 2];
    int nextCluster;
    if (currentCluster % 2 == 0) { //当前为偶数号数据簇，第一个字节作为低八位，第二个字节前4位作为高4位
        nextCluster = (firstByte) | ((secondByte & 0x0F) << 8);
    } else {
        nextCluster = (thirdByte << 4) | ((secondByte >> 4) & 0x0F);
    }
    return nextCluster;
}

void findSubDirectoryAndFile(queue<string> &q, string path, RootEntry &current) {
    if (current.DIR_Name[0] != '\xe5')  //若目录项第一个字节修改为e5，表示该目录已经删除
    {
        pair<string, char> p;
        string name = "";
        string extension = "";
        name = getName(current);
        extension = getExtension(current);
        trim(name);
        trim(extension);
        if (current.DIR_Attr == 0x10) {   //当前为文件夹
            p.first = name;
            p.second = 'r';
            if (name != "." && name != "..") {
                if (path == "/") {
                    q.push(path + name);        //加入下一层目录
                } else {
                    q.push(path + "/" + name);
                }
            }
            toPrint.push_back(p);
            nextLevel.push_back(current);
        } else if (current.DIR_Attr == 0x20) {  //当前为文件
            p.first = name + "." + extension;
            p.second = 'w';
            toPrint.push_back(p);
            nextLevel.push_back(current);
        }
    }
}

void cat(string path) {
    vector<string> hierarchicalDirectories;
    if (path.find("/") == 0) {
        hierarchicalDirectories = split(path.substr(1, path.size() - 1), "/");
    } else {
        hierarchicalDirectories = split(path, "/");
    }
    if (hierarchicalDirectories.size() == 1) {    //文件在根目录下
        int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //开始簇号是根目录区首簇
        RootEntry current;
        for (int i = 0; i < rootEntries.size(); i++) {        //先找到第一级目录，跳转到数据区再说
            string name = getName(rootEntries[i]);
            name += ".";
            name += getExtension(rootEntries[i]);
            if (name == hierarchicalDirectories[0] && rootEntries[i].DIR_Attr == 0x20) {
                startCluster = rootEntries[i].DIR_FstClus;
                current = rootEntries[i];
                break;
            } else {
                startCluster = -1;
            }
        }
        if (startCluster == -1) {
            throw "File Not Exist!";
        }
        //现在startCluster中保存了该文件在数据区的起始簇
        int currentCluster = startCluster;
        char *content = new char[current.DIR_FileSize];
        int restSize = current.DIR_FileSize;
        char *nextPointer = content;
        while (true) {
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
            if (restSize >= bpb.BPB_BytsPerSEC) {
                fin.read(nextPointer, bpb.BPB_BytsPerSEC);
                nextPointer += bpb.BPB_BytsPerSEC;
                restSize -= bpb.BPB_BytsPerSEC;
            } else {
                fin.read(nextPointer, restSize);
                break;
            }
            currentCluster = calNextCluster(currentCluster);
        }
        string s = "";
        for (int i = 0; i < current.DIR_FileSize; i++) {
            s += content[i];
        }
        s += "\n";
        whitePrint(s.c_str());
        //cout << endl;
        delete[] content;
    } else {
        int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //开始簇号是根目录区首簇
        RootEntry current;
        for (int i = 0; i < rootEntries.size(); i++) {        //先找到第一级目录，跳转到数据区再说
            string name = getName(rootEntries[i]);
            string extension;
            if (!(extension = getExtension(rootEntries[i])).empty()) {
                name += ".";
                name += extension;
            }
            if (name == hierarchicalDirectories[0]) {
                startCluster = rootEntries[i].DIR_FstClus;
                current = rootEntries[i];
                break;
            } else {
                startCluster = -1;
            }
        }
        if (startCluster == -1) {
            throw "File Not Exist!\n";
        }
        for (int i = 1; i < hierarchicalDirectories.size(); i++) {
            current = findCluster(hierarchicalDirectories[i], startCluster);
            startCluster = current.DIR_FstClus;    //由上一级目录开始的簇来寻找当前目录开始的簇
            if (startCluster == -1) { //当前目录不存在
                throw "Wrong Path!\n";
            }
        }

        if(current.DIR_Attr != 0x20){
            throw "Wrong Path!\n";
        }
        int currentCluster = startCluster;
        char *content = new char[current.DIR_FileSize];
        int restSize = current.DIR_FileSize;
        int hasRead = 0;
        while (true) {
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);

            if (restSize >= bpb.BPB_BytsPerSEC) {
                fin.read((content + hasRead), bpb.BPB_BytsPerSEC);
                hasRead += bpb.BPB_BytsPerSEC;
                restSize -= bpb.BPB_BytsPerSEC;
            } else {
                fin.read((content + hasRead), restSize);
                break;
            }
            currentCluster = calNextCluster(currentCluster);
        }
        string s = "";
        for (int i = 0; i < current.DIR_FileSize; i++) {
            s += content[i];
        }
        s += "\n";
        whitePrint(s.c_str());
        //cout << endl;
        delete[] content;
    }
}


void solveInput() {
    string command;
    //cout << ">";
    string s = ">";
    whitePrint(s.c_str());
    getline(cin, command);
    if (command.length() == 0) {
        return;
    }

    if (command == "exit") {
        // cout << "Exit Successfully!";
        whitePrint("Exit Successfully!\n");
        throw 1;
    }
    vector<string> splitedCommand = commandSplit(command);
    queue<string> q;  //保存路径
    if (splitedCommand[0] == "ls") {
        if (splitedCommand.size() == 1) {
            q.push("/");
            ls(q);        //无任何参数
        } else {
            for (int i = 1; i < splitedCommand.size(); i++) {     //将-ll参数全部替换为-l参数
//                if (splitedCommand[i] == "-ll") {
//                    splitedCommand[i] = "-l";
//                }
                string temp = splitedCommand[i];
                if(temp.substr(0, 1) == "-"){
                    if(temp.size() == 1){
                        throw "Wrong Command!\n";
                    }
                    for(int j = 1; j < temp.size(); j++){
                        if(temp.substr(j, 1) != "l"){
                            throw "Wrong Command\n";
                        }
                    }
                    splitedCommand[i] = "-l";
                }
            }

            string path = "";
            int countOfPath = 0;
            bool containL = false;
            for (int i = 1; i < splitedCommand.size(); i++) {
                if (splitedCommand[i] == "-l") {    //当前参数为"-l"
                    containL = true;
                } else {    //当前参数为合法目录名
                    path = splitedCommand[i];
                    countOfPath++;
                }
            }

            if (countOfPath > 1) {   //输入的目录数过多
                throw "Too Many Diretories\n";
            }

            if (containL) {
                if (countOfPath == 0) {    //无文件路径，默认为根目录
                    q.push("/");
                    lsl(q);
                } else {
                    q.push(path);
                    lsl(q);
                }
            } else {
                q.push(path);
                ls(q);
            }
        }
    } else if (splitedCommand[0] == "cat") {
        if (splitedCommand.size() >= 3) {
            throw "Too Many Parameters!\n";
        } else if (splitedCommand.size() == 1) {
            throw "Please Input File Path!\n";
        } else {
            cat(splitedCommand[1]);
        }
    } else {
        throw "Wrong Command!\n";
    }
    nextLevel.clear();
    toPrint.clear();
}

int main() {
    fin.open("../a.img", ios_base::binary | ios::in); //以二进制读取a.img文件
    if (!fin.is_open()) {
        //cout << "File Not Exist!" << endl;
        whitePrint("File Not Exist!\n");
    }

    //读取BPB
    fin.seekg(3, ios::beg);
    fin.read((char *) &bpb, sizeof(bpb));

    //读取fat1
    fat1 = new char[bpb.BPB_FATSz16 * bpb.BPB_BytsPerSEC];
    fin.seekg(bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSEC, ios::beg);
    fin.read(fat1, bpb.BPB_FATSz16 * bpb.BPB_BytsPerSEC);


    //读取RootEntry，最多224个目录项
    fin.seekg((bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16) * bpb.BPB_BytsPerSEC, ios::beg);
    for (int i = 0; i < bpb.BPB_RootEntCnt; i++) {
        fin.read((char *) &rootEntries[i], sizeof(RootEntry));
    }

    while (true) {
        try {
            solveInput();
        } catch (const char *s) {
            //cout << s << endl;
            whitePrint(s);
        } catch (int exitcode) {    //收到退出的命令
            fin.close();
            return 1;
        }
    }
}


vector<string> commandSplit(string &command) {
    vector<string> splitedCommand = split(command, " ");
    return splitedCommand;
}

vector<string> split(string str, string pattern) {
    int pos;
    vector<string> result;
    str += pattern; //扩展字符串以方便操作，确保pos不为-1
    int size = str.size();
    for (int i = 0; i < size; i++) {
        pos = str.find(pattern, i); //从下标i开始找第一个pattern
        if (pos < size) {
            string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}


void trim(string &s) {
    int index = 0;
    if (!s.empty()) {
        while ((index = s.find(" ", index)) != string::npos) {
            s.erase(index, 1);
        }
    }
}

void lsOut(vector<pair<string, char> > toPrint) {     //每凑够一行输出一次
    for (int i = 0; i < toPrint.size(); i++) {
        pair<string, char> &p = toPrint[i];
        if (p.second == 'r') {
            //cout << "\033[31m" << p.first << "  " << "\033[0m";
            string s = p.first + "  ";
            redPrint(s.c_str());
        } else {
            //cout << "\033[31m" << p.first << "  " << "\033[0m";
            string s = p.first + "  ";
            whitePrint(s.c_str());
        }
    }
    //cout << endl;
    whitePrint("\n");
}


string getName(RootEntry rootEntry) {
    string res = "";
    for (int i = 0; i < 8; i++) {
        if (rootEntry.DIR_Name[i] != ' ') {
            res += rootEntry.DIR_Name[i];
        }
    }
    return res;
}

string getExtension(RootEntry rootEntry) {
    string res = "";
    for (int i = 8; i < 11; i++) {
        if (rootEntry.DIR_Name[i] != ' ') {
            res += rootEntry.DIR_Name[i];
        }
    }
    return res;
}




