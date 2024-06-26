#include <iostream>
#include <unistd.h>
#include <fstream>
using namespace std;

int main() {
    ifstream ifs;
    ofstream ofs;

    ifs.open("index.html",ios::binary | ios::in);

    if(!ifs.is_open()) {
        cerr<<"文件打开失败"<<endl;
        return 1;
    }

    //  得到buf
    //  获取文件长度
    //  seekg(0,ios::end) 表示将文件指针自end位置偏移0字节--也就是置于文件尾
    ifs.seekg(0,ios::end);
    //  tellg() 表示从文件指针的位置到文件头位置的字节数
    size_t length = ifs.tellg();
    //  再将文件指针复位至文件开头以正常操作文件
    ifs.seekg(0,ios::beg);

    char* buf = new char[length];
    ifs.read(buf,length);

    ofs.open("index2.html",ios::binary | ios::out);
    ofs.write(buf,length);

    ofs.close();
    ifs.close();
    delete[] buf;
}
