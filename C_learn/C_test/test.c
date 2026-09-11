#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#define MAX_USERS 100
#define MAX_BOOKS 200
#define MAX_BORROWS 500
#define MAX_NOTICES 50
#define MAX_CATEGORY 50
#define MAX_LEN 128

typedef struct
{
    int id;
    char account[32], password[32], name[32], phone[24];
    int role, enabled; // role:1管理员 0读者; enabled 1启用 0禁用
} User;

typedef struct
{
    int id;
    char name[64];
} Category;

typedef struct
{
    int id, categoryId, stock, available;
    char title[64], author[40], publisher[64], intro[160];
} Book;

typedef struct
{
    int id, userId, bookId;
    time_t borrowTime, returnTime;
    int returned;
    double fine;
    int paid; // 0未缴费 1已缴费
} Borrow;

typedef struct
{
    int id;
    char title[80], content[300];
    time_t publishTime;
    int publisherId;
} Notice;

// 全局数据
User users[MAX_USERS] = {{1, "admin", "admin123", "管理员", "", 1, 1}, {2, "reader", "123456", "普通读者", "", 0, 1}};
Category categories[MAX_CATEGORY];
Book books[MAX_BOOKS] = {{1, 1, 5, 5, "C程序设计", "谭浩强", "清华大学出版社", "C语言基础教材"}};

Borrow borrows[MAX_BORROWS];
Notice notices[MAX_NOTICES];

int userCount = 2, bookCount = 1, borrowCount = 0, noticeCount = 0, categoryCount = 1;
int maxBorrow = 5, borrowDays = 30;
double finePerDay = 1.0;
int borrowAutoId = 1; //借阅记录独立ID计数器

void line(void) { puts("------------------------------------------------"); }

// 清空输入缓冲区残留换行
void clearStdin(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void input(const char *prompt, char *s, int n)
{
    printf("%s", prompt);
    fgets(s, n, stdin);
    s[strcspn(s, "\n")] = 0;
}

int number(const char *prompt)
{
    char s[32];
    input(prompt, s, sizeof(s));
    return atoi(s);
}

const char *date(time_t t)
{
    static char s[32];
    struct tm *p = localtime(&t);
    strftime(s, sizeof(s), "%Y-%m-%d", p);
    return s;
}

int findUser(int id)
{
    for (int i = 0; i < userCount; i++)
        if (users[i].id == id)
            return i;
    return -1;
}

int findUserByAccount(const char *acc)
{
    for (int i = 0; i < userCount; i++)
        if (strcmp(users[i].account, acc) == 0)
            return i;
    return -1;
}

int findBook(int id)
{
    for (int i = 0; i < bookCount; i++)
        if (books[i].id == id)
            return i;
    return -1;
}

int findCategory(int id)
{
    for (int i = 0; i < categoryCount; i++)
        if (categories[i].id == id)
            return i;
    return -1;
}

// 统计用户当前未归还的图书数量
int activeBorrows(int uid)
{
    int n = 0;
    for (int i = 0; i < borrowCount; i++)
        if (borrows[i].userId == uid && !borrows[i].returned)
            n++;
    return n;
}

// 查询某本书是否存在未归还借阅记录
int bookHasUnreturned(int bookId)
{
    for (int i = 0; i < borrowCount; i++)
    {
        if (borrows[i].bookId == bookId && !borrows[i].returned)
            return 1;
    }
    return 0;
}

// 计算罚款
double calcFine(Borrow *b)
{
    if (b->returned)
        return b->fine;
    time_t end = time(NULL);
    long sec = (long)(end - b->borrowTime);
    int days = sec / 86400;
    return days > borrowDays ? (days - borrowDays) * finePerDay : 0;
}

// ===== 分类管理 =====
void listCategory(void)
{
    line();
    puts("分类ID\t分类名称");
    for (int i = 0; i < categoryCount; i++)
    {
        printf("%d\t%s\n", categories[i].id, categories[i].name);
    }
}
void addCategory(void)
{
    if (categoryCount >= MAX_CATEGORY)
    {
        puts("分类数量已达上限！");
        return;
    }
    Category *cat = &categories[categoryCount];
    cat->id = categoryCount ? categories[categoryCount - 1].id + 1 : 1;
    input("输入分类名称：", cat->name, 64);
    categoryCount++;
    puts("分类新增成功");
}

// ===== 图书模块 =====
void listBooks(void)
{
    line();
    printf("%-4s %-18s %-12s %-4s %-4s\n", "ID", "书名", "作者", "库存", "可借");
    for (int i = 0; i < bookCount; i++)
        printf("%-4d %-18s %-12s %-4d %-4d\n", books[i].id, books[i].title, books[i].author, books[i].stock, books[i].available);
}

void searchBooks(void)
{
    char key[64];
    input("输入书名/作者关键词：", key, sizeof(key));
    int findFlag = 0;
    for (int i = 0; i < bookCount; i++)
    {
        if (strstr(books[i].title, key) || strstr(books[i].author, key))
        {
            printf("[%d] %s | 作者:%s | 可借:%d | %s\n", books[i].id, books[i].title, books[i].author, books[i].available, books[i].intro);
            findFlag = 1;
        }
    }
    if (!findFlag) puts("未找到匹配图书");
}

void addBook(void)
{
    if (bookCount >= MAX_BOOKS)
    {
        puts("图书数量已达上限，无法新增！");
        return;
    }
    Book *b = &books[bookCount];
    b->id = bookCount ? books[bookCount - 1].id + 1 : 1;
    input("书名：", b->title, 64);
    input("作者：", b->author, 40);
    input("出版社：", b->publisher, 64);
    input("简介：", b->intro, 160);
    b->stock = number("库存：");
    b->available = b->stock;
    b->categoryId = number("分类ID：");
    bookCount++;
    puts("添加成功。");
}

void deleteBook(void)
{
    int id = number("图书ID："), i = findBook(id);
    if (i < 0)
    {
        puts("图书不存在。");
        return;
    }
    if (bookHasUnreturned(id))
    {
        puts("本书尚有借出记录，禁止删除！");
        return;
    }
    // 前移删除，不是交换末尾，规避脏数据
    for (int j = i; j < bookCount - 1; j++)
    {
        books[j] = books[j + 1];
    }
    bookCount--;
    puts("删除成功。");
}

// ===== 借阅业务 =====
void borrowBook(User *u)
{
    if (activeBorrows(u->id) >= maxBorrow)
    {
        puts("已达到个人借阅上限！");
        return;
    }
    listBooks();
    int id = number("借阅图书ID："), i = findBook(id);
    if (i < 0 || books[i].available <= 0)
    {
        puts("图书不存在或无可借库存。");
        return;
    }
    Borrow *b = &borrows[borrowCount++];
    b->id = borrowAutoId++;
    b->userId = u->id;
    b->bookId = id;
    b->borrowTime = time(NULL);
    b->returned = 0;
    b->fine = 0;
    b->paid = 0;
    books[i].available--;
    puts("借阅成功。");
}

void returnBook(User *u)
{
    int rid = number("借阅记录ID：");
    for (int i = 0; i < borrowCount; i++)
    {
        if (borrows[i].id == rid && borrows[i].userId == u->id && !borrows[i].returned)
        {
            borrows[i].fine = calcFine(&borrows[i]);
            borrows[i].returned = 1;
            borrows[i].returnTime = time(NULL);

            int bidx = findBook(borrows[i].bookId);
            if (bidx != -1)
            {
                books[bidx].available++;
            }
            else
            {
                puts("警告：该记录关联图书不存在！");
            }
            printf("归还成功，产生罚款：%.2f元（未缴费）\n", borrows[i].fine);
            return;
        }
    }
    puts("未找到您有效的借阅记录。");
}

// 续借
void renewBook(User *u)
{
    int rid = number("借阅记录ID：");
    for (int i = 0; i < borrowCount; i++)
    {
        if (borrows[i].id == rid && borrows[i].userId == u->id && !borrows[i].returned)
        {
            borrows[i].borrowTime = time(NULL);
            borrows[i].fine = 0;
            puts("续借成功，重新计算借阅期限！");
            return;
        }
    }
    puts("无有效借阅记录，续借失败");
}

// 缴纳罚款
void payFine(User *u)
{
    int rid = number("借阅记录ID：");
    for (int i = 0; i < borrowCount; i++)
    {
        if (borrows[i].id == rid && borrows[i].userId == u->id && borrows[i].returned && borrows[i].paid == 0)
        {
            printf("待缴罚款：%.2f元\n", borrows[i].fine);
            borrows[i].paid = 1;
            puts("罚款缴纳完成！");
            return;
        }
    }
    puts("无待缴费记录");
}

void myBorrows(User *u)
{
    line();
    for (int i = 0; i < borrowCount; i++)
    {
        if (borrows[i].userId == u->id)
        {
            int bidx = findBook(borrows[i].bookId);
            char bookName[32] = "【图书丢失】";
            if (bidx != -1) strcpy(bookName, books[bidx].title);

            char status[32], payinfo[32];
            if (borrows[i].returned)
            {
                strcpy(status, "已归还");
                sprintf(payinfo, "罚款%.2f %s", borrows[i].fine, borrows[i].paid ? "已缴" : "待缴");
            }
            else
            {
                strcpy(status, "借阅中");
                double f = calcFine(&borrows[i]);
                sprintf(payinfo, "当前罚款:%.2f", f);
            }
            printf("记录%d | %s | 借于:%s | %s | %s\n", borrows[i].id, bookName, date(borrows[i].borrowTime), status, payinfo);
        }
    }
}

// ===== 公告 =====
void noticesMenu(void)
{
    line();
    if (!noticeCount)
    {
        puts("暂无公告。");
        return;
    }
    for (int i = 0; i < noticeCount; i++)
        printf("[%d] %s (%s)\n%s\n", notices[i].id, notices[i].title, date(notices[i].publishTime), notices[i].content);
}

void addNotice(User *u)
{
    if (noticeCount >= MAX_NOTICES)
    {
        puts("公告数量已满");
        return;
    }
    Notice *n = &notices[noticeCount];
    n->id = noticeCount + 1;
    n->publisherId = u->id;
    n->publishTime = time(NULL);
    input("公告标题：", n->title, 80);
    input("公告内容：", n->content, 300);
    noticeCount++;
    puts("发布公告成功。");
}

// ===== 用户管理（管理员）=====
void listAllUser(void)
{
    line();
    printf("%-4s %-12s %-10s %-6s\n", "ID", "账号", "姓名", "状态");
    for (int i = 0; i < userCount; i++)
        printf("%-4d %-12s %-10s %-6s\n", users[i].id, users[i].account, users[i].name, users[i].enabled ? "正常" : "禁用");
}

void addUser(void)
{
    if (userCount >= MAX_USERS)
    {
        puts("用户数量已达上限！");
        return;
    }
    char acc[32], pwd[32], name[32], phone[24];
    int role;
    input("账号：", acc, 32);
    if (findUserByAccount(acc) != -1)
    {
        puts("账号已存在！");
        return;
    }
    input("密码：", pwd, 32);
    input("姓名：", name, 32);
    input("手机号：", phone, 24);
    role = number("角色(1管理员,0读者):");

    User *nu = &users[userCount];
    nu->id = userCount ? users[userCount - 1].id + 1 : 1;
    strcpy(nu->account, acc);
    strcpy(nu->password, pwd);
    strcpy(nu->name, name);
    strcpy(nu->phone, phone);
    nu->role = role;
    nu->enabled = 1;
    userCount++;
    puts("新增用户成功");
}

void toggleUserEnable(void)
{
    int uid = number("用户ID：");
    int idx = findUser(uid);
    if (idx == -1)
    {
        puts("用户不存在");
        return;
    }
    users[idx].enabled = !users[idx].enabled;
    puts(users[idx].enabled ? "用户已启用" : "用户已禁用");
}

// ===== 统计 =====
void stats(void)
{
    int active = 0;
    for (int i = 0; i < borrowCount; i++)
        if (!borrows[i].returned)
            active++;
    line();
    printf("图书总数:%d，借阅记录:%d，当前借出:%d，用户数:%d\n", bookCount, borrowCount, active, userCount);
}

// ===== 菜单 =====
void adminMenu(User *u)
{
    int c;
    do
    {
        line();
        puts("=====管理员菜单=====");
        puts("1图书列表 2新增图书 3删除图书");
        puts("4分类管理 5用户管理 6发布公告");
        puts("7查看公告 8数据统计 9返回登录页 0退出程序");
        c = number("请选择：");
        if (c == 1) listBooks();
        else if (c == 2) addBook();
        else if (c == 3) deleteBook();
        else if (c == 4)
        {
            int sub;
            do
            {
                line();
                puts("【分类管理】1查看分类 2新增分类 0返回");
                sub = number("选择：");
                if (sub == 1) listCategory();
                else if (sub == 2) addCategory();
            } while (sub != 0);
        }
        else if (c == 5)
        {
            int sub;
            do
            {
                line();
                puts("【用户管理】1查看全部 2新增用户 3启用/禁用 0返回");
                sub = number("选择：");
                if (sub == 1) listAllUser();
                else if (sub == 2) addUser();
                else if (sub == 3) toggleUserEnable();
            } while (sub != 0);
        }
        else if (c == 6) addNotice(u);
        else if (c == 7) noticesMenu();
        else if (c == 8) stats();
        else if (c == 9)
        {
            puts("返回登录页面...");
            break;
        }
    } while (c != 0);
}

void readerMenu(User *u)
{
    int c;
    do
    {
        line();
        puts("=====读者菜单=====");
        puts("1图书列表 2搜索图书 3借阅图书");
        puts("4归还图书 5续借图书 6我的借阅记录");
        puts("7缴纳罚款 8查看公告 9返回登录页 0退出程序");
        c = number("请选择：");
        if (c == 1) listBooks();
        else if (c == 2) searchBooks();
        else if (c == 3) borrowBook(u);
        else if (c == 4) returnBook(u);
        else if (c == 5) renewBook(u);
        else if (c == 6) myBorrows(u);
        else if (c == 7) payFine(u);
        else if (c == 8) noticesMenu();
        else if (c == 9)
        {
            puts("返回登录页面...");
            break;
        }
    } while (c != 0);
}

int main(void)
{
    // 初始化默认分类
    categories[0].id = 1;
    strcpy(categories[0].name, "默认分类");

    puts("===== 图书馆管理系统 =====");
    for (;;)
    {
        char a[32], p[32];
        input("账号（输入0退出程序）：", a, 32);
        if (!strcmp(a, "0"))
            break;
        input("密码：", p, 32);
        int ok = -1;
        for (int i = 0; i < userCount; i++)
        {
            if (users[i].enabled && !strcmp(users[i].account, a) && !strcmp(users[i].password, p))
                ok = i;
        }
        if (ok < 0)
        {
            puts("账号或密码错误，或账号已禁用！");
            continue;
        }
        printf("欢迎，%s！\n", users[ok].name);
        if (users[ok].role)
            adminMenu(&users[ok]);
        else
            readerMenu(&users[ok]);
    }
    puts("系统已退出。");
    return 0;
}
