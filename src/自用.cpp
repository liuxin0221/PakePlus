#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_RECORD 100
#define MAX_STR 256

typedef struct {
    char imgPath[MAX_STR];
    char note[MAX_STR];
    char tag[MAX_STR];
    char timeStr[MAX_STR];
    int valid;
} Record;

Record list[MAX_RECORD];
int count = 0;

void getNowTime(char *buf) {
    time_t t = time(NULL);
    strftime(buf, MAX_STR, "%Y-%m-%d %H:%M:%S", localtime(&t));
}

void addRecord() {
    if(count >= MAX_RECORD) {
        printf("记录已满！\n");
        return;
    }
    Record *r = &list[count];
    printf("输入图片文件路径：");
    scanf("%s", r->imgPath);
    getchar();
    printf("输入备注：");
    gets(r->note);
    printf("输入标签(高数/英语等)：");
    gets(r->tag);
    getNowTime(r->timeStr);
    r->valid = 1;
    count++;
    printf("? 添加成功\n");
}

void showAll() {
    printf("\n====所有学习记录====\n");
    for(int i=0;i<count;i++){
        if(list[i].valid){
            printf("[%d] 时间:%s\n",i,list[i].timeStr);
            printf("路径:%s\n备注:%s\n标签:%s\n------\n",
                   list[i].imgPath,list[i].note,list[i].tag);
        }
    }
}

void searchByTag() {
    char tag[MAX_STR];
    printf("输入要查找的标签：");
    gets(tag);
    printf("\n===标签【%s】检索结果===\n",tag);
    for(int i=0;i<count;i++){
        if(list[i].valid && strstr(list[i].tag,tag)){
            printf("[%d] %s | %s\n备注:%s\n",list[i].timeStr,list[i].imgPath,list[i].note);
        }
    }
}

void delRecord(){
    int id;
    printf("输入删除记录ID：");
    scanf("%d",&id);
    if(id>=0 && id<count && list[id].valid){
        list[id].valid = 0;
        printf("删除成功\n");
    }else printf("ID无效\n");
}

int main() {
    int op;
    while(1){
        printf("\n====学习相册【C控制台版】====\n");
        printf("1 添加记录\n2 查看全部\n3 标签检索\n4 删除记录\n0 退出\n请输入选项：");
        scanf("%d",&op);
        switch(op){
            case 1: addRecord(); break;
            case 2: showAll(); break;
            case 3: searchByTag(); break;
            case 4: delRecord(); break;
            case 0: return 0;
            default: printf("输入错误\n");
        }
    }
    return 0;
}
