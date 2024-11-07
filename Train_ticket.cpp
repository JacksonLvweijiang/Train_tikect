﻿#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>
#include<stdio.h>
#include<windows.h>
#include<stdlib.h>
#include<cstring>
#include "mysql_driver.h"
#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/statement.h"
#include "cppconn/prepared_statement.h"
#include "cppconn/metadata.h"
#include "cppconn/exception.h"
//-*- coding: GBK -*-

//MySQL连接设置
#define TCP "tcp://localhost:3306/new_database"
#define USERNAME "root"
#define PASSWORD "123456"

using namespace std;
using namespace sql;

sql::mysql::MySQL_Driver* mysql_driver;
sql::Connection* mysql_conn;

//连接MySQL
sql::Statement* mysql_connection(sql::mysql::MySQL_Driver* driver = mysql_driver, sql::Connection* conn = mysql_conn)
{
    driver = 0;
    conn = 0;
    try
    {
        driver = sql::mysql::get_mysql_driver_instance();
        conn = driver->connect(TCP, USERNAME, PASSWORD);
        cout << "连接成功" << endl;
    }
    catch (...)
    {
        cout << "连接失败" << endl;
    }
    sql::Statement* stat = conn->createStatement();
    stat->execute("set names 'gbk'");
    return stat;
}
//连接到数据库
sql::Statement* database_stat = mysql_connection(mysql_driver, mysql_conn);
ResultSet* database_result;


//一下几个是辅助函数
//1,将GBK转化为UTF-8
std::string gbk_to_utf8(const std::string& gbkStr)
{
    std::wstring_convert<std::codecvt_byname<wchar_t, char, std::mbstate_t>> conv1(new std::codecvt_byname<wchar_t, char, std::mbstate_t>(".936"));
    std::wstring wstr = conv1.from_bytes(gbkStr);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv2;
    return conv2.to_bytes(wstr);
}
//2,跳转的指定行列
void gotoxy(int x, int y)
{
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord;
    coord.X = x;
    coord.Y = y;

    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
    coord.X += sbi.srWindow.Left;
    coord.Y += sbi.srWindow.Top;

    SetConsoleCursorPosition(hConsoleOutput, coord);
}
//3,用于清除界面上的文字
void clearScreen(int x0, int y0, int x1, int y1) {
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD topLeft = { x0, y0 };
    DWORD length = (x1 - x0 + 1) * (y1 - y0 + 1);
    DWORD written;

    // 将缓冲区中指定区域的所有字符都替换为空格。
    FillConsoleOutputCharacter(hConsoleOutput, ' ', length, topLeft, &written);

    // 将缓冲区中指定区域的所有字符属性都替换为默认值。
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
    FillConsoleOutputAttribute(hConsoleOutput, sbi.wAttributes, length, topLeft, &written);

    // 将光标移回指定位置。
    SetConsoleCursorPosition(hConsoleOutput, topLeft);
}
//4,将工作文件夹下的指定文件中的内容读取为string格式
std::string readFileToString(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    if (file) {
        std::cout << "文件读取成功" << endl;
        buffer << file.rdbuf();
        file.close();
    }
    else {
        std::cout << "无法打开文件" << endl;
    }
    return buffer.str();
}
//5,展示查询记录
void show_record()
{
    gotoxy(0, 22);
    clearScreen(0, 22, 100, 42);
    gotoxy(0, 22);
    int cnt = 0;
    while (database_result->next()) {
        cnt++;
        if (cnt > 5) {
            printf("...\n");
            return;
        }
        //逐条展示查询结果
        cout << cnt << "  \t"
            << database_result->getString("train_number") << "  \t"
            << database_result->getString("starting_station") << "  \t"
            << database_result->getString("terminal_station") << "  \t"
            << database_result->getString("departure_time") << "  \t"
            << database_result->getString("arrival_time") << "  \t"
            << database_result->getString("total_votes") << "  \t"
            << database_result->getString("remaining_votes") << "  \t"
            << database_result->getString("ticket_price") << endl;
    }
    if (!cnt) {
        printf("没有查询到相关记录");
    }
    return;
}
//6,查询订票记录
void show_book(const string order_id)
{
    gotoxy(0, 21);
    printf("编号    订票ID  车次\n");
    clearScreen(0, 22, 100, 42);
    string ins = "SELECT * FROM new_database.reservation where order_id= ";
    ins = ins + '\'' + order_id + '\'';
    database_result = database_stat->executeQuery(ins);
    int cnt = 0;
    while (database_result->next()) {
        cnt++;
        if (cnt > 5) {
            printf("...\n");
            return;
        }
        cout << database_result->getString("ID") << "  \t"
            << database_result->getString("order_id") << "  \t"
            << database_result->getString("train_number") << endl;
    }
    if (!cnt) {
        printf("没有查询到相关记录");
    }
    return;
}
//7,执行MySQL语句
void file_sql(const std::string& filename)
{
    string ins = readFileToString(filename);
    database_stat->execute(ins);
}
//操作函数
//在表中查找
void select_ticket()
{
    database_result = database_stat->executeQuery("SELECT * FROM ticket");
    show_record();
}
//筛选函数
/*
* 筛选参数说明
* op值 ins内容      作用
* 1    车次         查询车次
* 2    起始站名称   查询起始站为所给城市的所有车次
* 3    终点站名称   查询终点站为所给城市的所有车次
* 4    发车时间区间 查询发车时间为所给区间的所有车次(格式->[00:00:00-00:00:00])
* 5    到站时间区间 查询到站时间为所给区间的所有车次(格式->[00:00:00-00:00:00])
* 6    票价区间     查询票价为所给区间的所有车次(格式->[0.00-0.00])
*/
void fliter_ticket(const int& op, const string& ins)
{
    string head = "SELECT * FROM ticket WHERE ";
    //查询车次,查询起始站为所给城市的所有车次,查询终点站为所给城市的所有车次
    if (op == 1 || op == 2 || op == 3) {
        if (op == 1) { head += "train_number = "; }
        else if (op == 2) { head += "starting_station = "; }
        else if (op == 3) { head += "terminal_station = "; }
        head += '\'' + ins + '\'';
    }
    //查询发车时间为所给区间的所有车次,查询到站时间为所给区间的所有车次
    else if (op == 4 || op == 5 || op == 6) {
        string be = '\'' + ins.substr(0, ins.find('-')) + '\'', ed = '\'' + ins.substr(ins.find('-') + 1, ins.length() - ins.find('-')) + '\'';
        if (op == 4) { head += "departure_time "; }
        else if (op == 5) { head += "arrival_time "; }
        else if (op == 6) { head += "ticket_price "; }
        head += "BETWEEN " + be + " AND " + ed;
    }
    head += ';';
    database_result = database_stat->executeQuery(head);
    show_record();
    return;
}
/*
* 参数说明
* 1, train_number 每条数据所定的车次
* 2, order_id 订票者的id
*/
void order_ticket(const string& train_number, const string& order_id)
{
    string head = "INSERT INTO reservation (train_number , order_id) VALUES (\'" + train_number + "\',\'" + order_id + "\');";
    database_stat->execute(head);
    file_sql("update_order.in");
    return;
}
/*
* 参数说明
* 1, 修改车次
* 2, 修改起始站
* 3, 修改终点站
* 4, 修改发车时间
* 5, 修改到站时间
* 6, 修改总票数
* 7, 修改票价
*/
void modify(const int& op, const string& content, const string& key)
{
    string head = "UPDATE users SET ", fields;
    if (op == 1) { fields = "train_number"; }
    else if (op == 2) { fields = "starting_station"; }
    else if (op == 3) { fields = "terminal_station"; }
    else if (op == 4) { fields = "departure_time="; }
    else if (op == 5) { fields = "arrival_time"; }
    else if (op == 6) { fields = "total_votes"; }
    else if (op == 7) { fields = "ticket_price"; }
    head.append(fields);
    head.append("='" + content + '\'' + " where train_number= " + '\'' + key + '\'');
    database_stat->execute(head);
    return;
}

void inquire();
void modify_record();
void BookTicket();
void ImportExport();
void title();
int menu();

//辅助函数
void gotoxy(int x, int y);
void output_board(int i, int j, int chang, int kuan);
void clear();
void Color();

int Menu, Menu1 = 1, Menu2 = 2, Menu3 = 3, Menu4 = 4;
int Inquire, Book, IE;
char null;

int main()
{

    //设置窗口的样式
    Color();
    //显示标题界面
    title();
    //显示菜单栏
    while (menu() != -1);
    //这里填入展示符合的车票SearchTrain函数并展示（注意用gotoxy避开边框线）
    //在展示完毕后决定是否订票BookTicket函数
    //信息核对Modify函数
    //展示最后的火车票并保存
    if (mysql_conn != 0)
    {
        delete mysql_conn;
    }
}

void Color()//字体颜色设置函数
{
    HANDLE hOut;
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hOut,
        FOREGROUND_BLUE |
        FOREGROUND_INTENSITY |
        COMMON_LVB_UNDERSCORE);
}

void title()
{
    output_board(0, 0, 100, 20);
    printf("                                                                                                       \n");
    printf("                                                                                                       \n");
    printf("                                                       ________________________                        \n");
    printf("               {           _________________                   |       |                               \n");
    printf("                { /                    |                _______|_______|______                         \n");
    printf("                |  |                   |                |      |       |      |                        \n");
    printf("               /   |                   |                |      |       |      |                        \n");
    printf("              /    |                   |                |——————|———————|——————|     \n");
    printf("                   |                   |                     -------|------                            \n");
    printf("                   |                   |                  __________|__________                        \n");
    printf("                   |                   |                            |                                  \n");
    printf("                   |                .  |                      /  .  |    !                             \n");
    printf("                   |  /             !  |                    /     ! |     !                            \n");
    printf("                    /                ! |                   /       ||      !                           \n");
    printf("                                                                    .                                  \n");
    printf("                                                                                                       \n");
    printf("                                         (按任意建以继续)                                               \n");
    getchar();
    clear();
}

void clear()//辅助函数，用于清除屏幕（保留边框，并将光标移到（1,1））
{
    system("cls");
    output_board(0, 0, 100, 20);
    gotoxy(1, 1);
}

void output_board(int i, int j, int chang, int kuan)//辅助函数 用于生成一个边框的函数
{
    int last_line = i + kuan - 1;
    int last_col = j + chang - 1;
    int first_j = j;
    int first_i = i;
    for (; i <= last_line; i++) {
        j = first_j;
        for (; j <= last_col; j++)
        {
            gotoxy(j, i);
            if (i == first_i || i == last_line) {
                printf("-");
            }
            else if (j == first_j || j == last_col) {
                printf("|");
            }
        }
    }
    gotoxy(1, 1);

}

int menu()//菜单函数
{

    while (1) {
        gotoxy(1, 1);    // 恢复控制台输出的位置    
        system("cls");
        output_board(0, 0, 100, 20);
        printf("菜单:");
        gotoxy(1, 2);
        //Sleep(500);
        printf("1.查询");
        gotoxy(1, 3);
        //Sleep(500);
        printf("2.订票/修改");
        gotoxy(1, 4);
        //Sleep(500);
        printf("3.导入/导出");
        gotoxy(1, 5);
        //Sleep(500);
        printf("4.退出");
        gotoxy(1, 6);
        //Sleep(500);
        printf("请选择您的服务(输入数字):");
        cin >> Menu;
        if (Menu == Menu1)
            inquire();
        else if (Menu == Menu2)
            modify_record();
        else if (Menu == Menu3)
            ImportExport();
        else if (Menu == Menu4)
            return -1;
        else {
            gotoxy(1, 7);
            printf("请输入正确的数字");
            gotoxy(26, 6);
        }
    }
}

void inquire()
{
    Inquire = 0;
    clear();
    printf("查询条件,*代表任意");
    gotoxy(1, 2);
    //Sleep(300);
    printf("1.始发-终点站:*-*");
    gotoxy(1, 3);
    //Sleep(300);
    printf("2,发车时间区间:*-*");
    gotoxy(1, 4);
    //Sleep(300);
    printf("3,到站时间区间:*-*");
    gotoxy(1, 5);
    //Sleep(300);
    printf("4,票价:*-*");
    gotoxy(1, 6);
    //Sleep(300);
    printf("5.车次:*");
    gotoxy(1, 7);
    //Sleep(300);
    printf("6.确认查询");
    gotoxy(1, 8);
    //Sleep(300);
    printf("7.返回");
    gotoxy(1, 9);
    //Sleep(300);
    printf("请选择您的服务(输入数字):");
    //根据所输入的指令执行命令
    string insx, insy, select_ins = "SELECT * FROM new_database.ticket ";
    bool flag = 0;
    while (1) {
        clearScreen(26, 9, 30, 9);//清除之前输入的数字命令
        clearScreen(1, 10, 30, 10);
        clearScreen(1, 11, 30, 11);//清除查询指导
        gotoxy(0, 21);
        cout << ("编号    车次    出发站  终点站  发车时间        到达时间        总票数  剩余    票价");
        if (Inquire == 0)select_ticket();
        gotoxy(26, 9);
        cin >> Inquire;
        gotoxy(1, 10);
        //始发-终点站

        if (Inquire == 1) {
            printf("请输入始发站:");
            cin >> insx;
            gotoxy(1, 11);
            printf("请输入终点站:");
            cin >> insy;
            if (!flag) {
                flag = 1;
                select_ins = select_ins + "where starting_station=" + '\'' + insx + '\'' + " AND" + " terminal_station=" + '\'' + insy + '\'';
            }
            else {
                select_ins = select_ins + " AND starting_station=" + '\'' + insx + '\'' + " AND" + " terminal_station=" + '\'' + insy + '\'';
            }
            clearScreen(15, 2, 30, 2);
            gotoxy(15, 2);
            cout << insx << '-' << insy;
        }
        //发车时间区间
        else if (Inquire == 2) {
            printf("请输时间:");
            cin >> insx;
            gotoxy(1, 11);
            printf("请输时间:");
            cin >> insy;
            if (!flag) {
                select_ins = select_ins + "where departure_time BETWEEN " + '\'' + insx + '\'' + "AND" + '\'' + insy + '\'';
            }
            else {
                flag = 1;
                select_ins = select_ins + " AND departure_time BETWEEN " + '\'' + insx + '\'' + "AND" + '\'' + insy + '\'';
            }
            clearScreen(16, 3, 30, 3);
            gotoxy(16, 3);
            cout << insx << '-' << insy;
        }
        //到站时间区间
        else if (Inquire == 3) {
            printf("请输时间:");
            cin >> insx;
            gotoxy(1, 11);
            printf("请输时间:");
            cin >> insy;
            if (!flag) {
                select_ins = select_ins + "where arrival_time BETWEEN " + '\'' + insx + '\'' + "AND" + '\'' + insy + '\'';
            }
            else {
                flag = 1;
                select_ins = select_ins + " AND arrival_time BETWEEN " + '\'' + insx + '\'' + "AND" + '\'' + insy + '\'';
            }
            clearScreen(16, 4, 30, 4);
            gotoxy(16, 4);
            cout << insx << '-' << insy;
        }
        //票价
        else if (Inquire == 4) {
            printf("请输入最低价格:");
            cin >> insx;
            gotoxy(1, 11);
            printf("请输入最高价格:");
            cin >> insy;
            if (!flag) {
                select_ins = select_ins + "where ticket_price BETWEEN " + '\'' + insx + '\'' + "AND" + '\'' + insy + '\'';
            }
            else {
                flag = 1;
                select_ins = select_ins + " AND ticket_price BETWEEN " + '\'' + insx + '\'' + "AND" + '\'' + insy + '\'';
            }
            clearScreen(8, 5, 30, 5);
            gotoxy(8, 5);
            cout << insx << '-' << insy;
        }
        //车次
        else if (Inquire == 5) {
            printf("请输入车次:");
            cin >> insx;
            if (!flag) {
                select_ins = select_ins + "where train_number=" + '\'' + insx + '\'';
            }
            else {
                flag = 1;
                select_ins = select_ins + " AND train_number = " + '\'' + insx + '\'';
            }
            clearScreen(8, 6, 30, 6);
            gotoxy(8, 6);
            cout << insx;
        }
        //执行查询命令
        else if (Inquire == 6) {
            try {
                database_result = database_stat->executeQuery(select_ins);
                show_record();
            }
            catch (const exception& e) {
                gotoxy(1, 10);
                printf("条件格式错误请检查条件");
            }

        }
        //返回
        else if (Inquire == 7) {
            return;
        }
        else {
            gotoxy(1, 12);
            printf("请输入正确的数字");
            gotoxy(26, 10);
        }
    }
}

void modify_record() {
    clear();
    printf("1,订票");
    gotoxy(1, 2);
    printf("2,查询订票记录");
    gotoxy(1, 3);
    printf("3,删除");
    gotoxy(1, 4);
    printf("4,修改");
    gotoxy(1, 5);
    printf("5,返回");
    gotoxy(1, 6);
    printf("请选择您的服务(输入数字):");
    while (1) {
        clearScreen(26, 6, 50, 6);
        gotoxy(26, 6);
        cin >> Inquire;
        clearScreen(1, 7, 50, 7);//清除之前的指导文字
        clearScreen(1, 8, 50, 8);
        clearScreen(1, 9, 50, 9);
        clearScreen(1, 10, 50, 10);
        gotoxy(1, 7);
        if (Inquire == 1) {
            string train_number, order_id, op, re;
            printf("请输入所定车次:");
            cin >> train_number;
            gotoxy(1, 9);
            printf("请输入订票ID:");
            cin >> order_id;
            gotoxy(1, 10);
            printf("是否确认订票(输入y代表确认,其他代表否):");
            cin >> op;
            if (op == "y") {
                order_ticket(train_number, order_id);
                gotoxy(1, 11);
                printf("(订票成功)\n");
            }
            else {
                gotoxy(1, 11);
                printf("(订票失败)\n");
            }
        }
        else if (Inquire == 2) {
            string order_id, op, re;
            printf("请输入ID:");
            cin >> order_id;
            show_book(order_id);
        }
        else if (Inquire == 3) {
            string tmp_train_number, ins = "DELETE FROM new_database.ticket WHERE train_number= ";
            printf("请输入车次:");
            cin.ignore();
            cin >> tmp_train_number;
            ins.append('\'' + tmp_train_number + '\'');
            try {
                database_stat->execute(ins);
                gotoxy(1, 8);
                printf("删除成功");
            }
            catch (...) {
                gotoxy(1, 8);
                printf("删除失败");
            }
        }
        else if (Inquire == 4) {
            string tmp_train_number, content; int op;
            printf("请输入车次:");
            cin.ignore();
            cin >> tmp_train_number;
            fliter_ticket(1, tmp_train_number);
            gotoxy(0, 21);
            cout << ("编号    车次    出发站  终点站  发车时间        到达时间        总票数  剩余    票价");
            show_record();
            gotoxy(1, 8);
            printf("请输入要修改的数据的字段编号,从左到右:车次(1),出发站(2)...:");
            cin >> op;
            gotoxy(1, 9);
            printf("请输入要修改的数据内容:");
            cin >> content;
            try {
                modify(op, content, tmp_train_number);
                gotoxy(1, 10);
                printf("修改成功");
            }
            catch (...) {
                gotoxy(1, 10);
                printf("修改失败");
            }
        }
        else if (Inquire == 5) {
            return;
        }
        else {
            printf("请输入正确的数字");
        }
    }

}

void BookTicket()//订票函数
{
    clear();
    string train_number, order_id, op, re;
    printf("请输入您要订的车次:");
    cin >> train_number;
    gotoxy(1, 2);
    printf("请输入您的ID:");
    cin >> order_id;
    gotoxy(1, 3);
    printf("是否确认订票(输入y代表确认,其他代表否):");
    cin >> op;
    if (op == "y") {
        order_ticket(train_number, order_id);
        gotoxy(1, 4);
        printf("(按任意建以继续)\n");
        gotoxy(1, 5);
        cin >> re;
        return;
    }
    else return;
}

void ImportExport()//本地导入导出函数
{
    clear();
    printf("1.本地导入");
    gotoxy(1, 2);
    //Sleep(500);
    printf("2.导出");
    gotoxy(1, 3);
    //Sleep(500);
    printf("3.返回");
    gotoxy(1, 4);
    //Sleep(500);
    printf("请选择您的服务(输入数字):");
    while (1) {
        clearScreen(26, 4, 50, 4);//
        cin >> IE;
        clearScreen(1, 5, 60, 5);//
        clearScreen(1, 6, 60, 6);//
        clearScreen(1, 7, 60, 7);//
        gotoxy(1, 5);
        if (IE == 1) {
            string pos, sst, sline, temp = "INSERT INTO ticket (train_number,starting_station,terminal_station,departure_time,arrival_time,total_votes,remaining_votes,ticket_price)VALUES ";
            printf("请输入需要导入的文件名称(请放置在工作目录下):");
            cin >> pos;
            gotoxy(1, 6);
            sst = readFileToString(pos);
            std::istringstream iss(sst);//创建输入流
            if (!sst.empty()) {
                int scnt = 0, fcnt = 0;
                while (std::getline(iss, sline)) { // 逐行读取输入流
                    try {
                        sline = temp + '(' + sline + ')';
                        database_stat->execute(sline);
                        scnt++;
                    }
                    catch (...) {
                        fcnt++;
                    }
                }
                gotoxy(1, 7);
                printf("成功输入%d条数据,输入失败%d条数据", scnt, fcnt);
            }
        }
        else if (IE == 2) {
            string fname;
            printf("请输入保存文件名称:");
            cin >> fname;
            gotoxy(1, 6);
            try {
                ofstream file(fname);
                database_result = database_stat->executeQuery("SELECT * FROM ticket");
                while (database_result->next()) {
                    file << '\'' << database_result->getString("train_number") << '\'' << ','
                        << '\'' << database_result->getString("starting_station") << '\'' << ','
                        << '\'' << database_result->getString("terminal_station") << '\'' << ','
                        << '\'' << database_result->getString("departure_time") << '\'' << ','
                        << '\'' << database_result->getString("arrival_time") << '\'' << ','
                        << database_result->getString("total_votes") << ','
                        << database_result->getString("remaining_votes") << ','
                        << database_result->getString("ticket_price") << endl;
                }
                printf("导出成功");
            }
            catch (...) {
                printf("导出失败");
            }

        }
        else if (IE == 3) {
            return;//返回
        }
        else {
            printf("请输入正确的数字");
        }
    }


}