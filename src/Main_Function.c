////////////////////////////////////////////////////////////////////////
//                                                                    //
//      时间:2022-5-27                                                //
//      作者:lee                                                      //
//      内容:实现第一阶段项目的主要功能函数                           //
//                                                                    //
//                                                                    //
////////////////////////////////////////////////////////////////////////

#include "Main_Function.h"

#define WEATHER_FILE "./tmp.txt"


//============================全局变量定义============================//
//定义两个普通全局变量，让工程内可见
extern int fd_lcd;
extern int fd_tc;

extern unsigned long pos_x;
extern unsigned long pos_y;

extern unsigned int out_x;
extern unsigned int out_y; // 松手坐标值
extern unsigned int pre_x;

extern unsigned int x_diff;

//伪状态机
extern bool is_playing_video;
extern bool change_bmp;

int * map;

//获取屏幕的固定属性
extern struct fb_fix_screeninfo fixinfo;
extern struct fb_var_screeninfo varinfo; // 可变属性
extern struct input_event pos_buf;
extern struct touch_state tc_state;
/* unsigned long T_WIDTH = 0;
unsigned long T_HEIGHT = 0;

unsigned long V_WIDTH  = 0;
unsigned long V_HEIGHT = 0;
unsigned long BPP = 0; */


//============================全局变量定义============================//

//功  能:申请映射内存
int * Map_Init(){

    //获取LCD设备硬件var属性
    get_varinfo();      
    unsigned long T_WIDTH = varinfo.xres;
    unsigned long T_HEIGHT = varinfo.yres;

    unsigned long V_WIDTH  = varinfo.xres_virtual;
    unsigned long V_HEIGHT = varinfo.yres_virtual;
    unsigned long BPP = varinfo.bits_per_pixel;

    //申请虚拟显存
    map = (int *)mmap(NULL, (T_WIDTH * T_HEIGHT * BPP/8), PROT_READ | PROT_WRITE, MAP_SHARED, fd_lcd, 0);
    if(map == MAP_FAILED){
        perror("申请映射内存失败");
        return NULL;
    }
    printf("已申请 1*(%ld*%ld*%ld) 大小的映射内存空间\n", T_WIDTH, T_HEIGHT, BPP/8);
    return map;
}


//功  能:收集目录内相关(.mp3/bmp(拓展.jpg)文件)
//       并且存入相关链表中
unsigned int collect_file(const char * Path, P_node my_list){

    //设定文件计数器
    int count = 0;

    //打开目录
    DIR * dir = opendir(Path);
    if(dir == NULL){
        // fprintf(stderr, "打开%s文件错误:\t%s\n", Path, strerror(errno));
        return 0;
    }

    while(1){

        //创建目录项结构体(放在数据段，只声明一次)
        struct dirent *ep = NULL;
        if( (ep = readdir(dir)) == NULL ){
            // fprintf(stderr, "读取目录 \"%s\" 错误:\t %s\n", Path, strerror(errno));
            break;
        }
        //首先跳过隐藏文件、当前目录以及上一级目录
        if(ep->d_name[0] == '.')    continue;

        //创建文件信息结构体(放在数据段，只声明一次)
        static struct stat file_stat;
        //获取文件的状态
        stat(Path, &file_stat);

        //如果是目录，则深入检索
        if( /*ep->d_type == 4*/(S_ISDIR(file_stat.st_mode)) ){
        
            //重新整理路径/文件名
            char path_buf[257];
            memset(path_buf, 0, 257);
            snprintf(path_buf, 257, "%s/%s", Path, ep->d_name);
            count += collect_file(path_buf, my_list);
        }
        //定义临时结构体变量
        data_type tmp;
        //如果是普通文件，则判断是否为.bmp文件，如果是则将文件名放进链表中
        if( !(S_ISREG(file_stat.st_mode)) ){
            //存储bmp文件
            if(IS_BMPfile(ep->d_name)){
                
                //将名字重新拼接
                snprintf(tmp.name, 257, "%s/%s", Path, ep->d_name);
                tmp.type = 'b';
                //创建新节点
                P_node new = new_node(tmp);
                //将新节点插入链表
                node_2_list(my_list, new, 1);
                count ++;
            }
            //存储mp3文件
            else if(IS_MP3file(ep->d_name)){
                //重复以上操作
                snprintf(tmp.name, 257, "%s/%s", Path, ep->d_name);
                tmp.type = 'm';
                P_node new = new_node(tmp);
                node_2_list(my_list, new, 1);
                count ++;
            }
            //存储avi文件
            else if(IS_AVI_file(ep->d_name)){
                snprintf(tmp.name, 257, "%s/%s", Path, ep->d_name);
                tmp.type = 'a';
                P_node new = new_node(tmp);
                node_2_list(my_list, new, 1);
                count ++;
            }

            continue;
        }

    }

    return count;
}


//功  能:判断是否为bmp图片
bool IS_BMPfile(char * name){

    char * tmp = NULL;
    if(tmp = strrchr(name, '.')){
        if(strcmp(tmp, ".bmp") == 0){
            printf("当前文件%s是BMP文件\n", name);
            return true;
        }
    }
    return false;
}

//功  能:判断是否为mp3文件
bool IS_MP3file(char * name){

    char * tmp = NULL;
    if(tmp = strrchr(name, '.')){
        if(strcmp(tmp, ".mp3") == 0){
            printf("当前文件%s是MP3文件\n", name);
            return true;
        }
    }
    return false;
}

//功  能:判断是否为avi文件
bool IS_AVI_file(char * name){
    char * tmp = NULL;
    if(tmp = strrchr(name, '.')){
        if(strcmp(tmp, ".avi") == 0){
            printf("当前文件%s是avi文件\n",name);
            return true;
        }
    }
    return false;
}

//功  能:操作系统
void my_system(P_node my_list){

    //首先找到工程界面
    P_node lock = search_2_list(my_list, "lock");
    P_node proj = search_2_list(my_list, "project");
    mem_Init();
flag:
    //进入锁界面的函数
    display_node(lock);

    if(project_lock()){
        //显示主界面
        display_node(proj);
        
        //打开相册
        while(1){
            //相册
            if(pos_x > 70 && pos_x < 205 && pos_y > 105 && pos_y < 260 ){
                pos_x = 0;
                pos_y = 0;
                printf("打开相册!\n");
                show_album(my_list);
                display_node(proj);
            }
            //音乐
            if(pos_x > 360 && pos_x < 500 && pos_y > 90 && pos_y < 270){
                pos_x = 0;
                pos_y = 0;
                printf("打开网抑云!\n");
                play_music(my_list);
                display_node(proj);
            }
            //视频
            if(pos_x > 75 && pos_x < 205 && pos_y > 280 && pos_y < 470){
                pos_x = 0;
                pos_y = 0;
                printf("打开爱奇艺!\n");
                play_video(my_list);
                display_node(proj);
            }
            //锁屏，返回锁界面
            if(pos_x > 700 && pos_x < 800 && pos_y > 0 && pos_y < 85){
                pos_x = 0;
                pos_y = 0;
                printf("注销。。。\n");
                goto flag;
            }
        }
    }
}



//功  能:播放音乐
void play_music(P_node my_list){
    //找到音乐界面(播放)
    P_node music_play  = search_2_list(my_list, "music_play");
    P_node music_pause = search_2_list(my_list, "music_pause");
    //找到第一个音乐文件
    P_node first_music = search_2_list(my_list, "1.mp3");
    display_node(music_play);
    
    //控制播放暂停标志位
    int flag = 0;
    P_node tmp = first_music;
    char str[257];
    memset(str, 0, 257);
    while(1){

        //播放/暂停
        if( pos_x > 260 && pos_x < 330 && pos_y > 405 && pos_y < 480 ){
            pos_x = 0;
            pos_y = 0;
            //一开始先从第一首播放
            if(flag == 0){
                printf("播放音乐\n");
                snprintf(str, 257, "madplay %s &", first_music->Data.name);
                system(str);
                display_node(music_pause);
            }

            //flag为偶数时继续播放
            else if( (flag % 2) == 0 && flag != 0){
                printf("继续播放\n");
                system("killall -SIGCONT madplay");
                display_node(music_pause);
            }

            //flag为奇数时暂停播放
            else if( (flag % 2) == 1 ){
                printf("暂停播放\n");
                display_node(music_play);
                system("killall -SIGSTOP madplay");
            }
            flag ++;
        }

        //上一首
        if(pos_x > 60 && pos_x < 130 && pos_y > 405 && pos_y <480){

            pos_x = 0;
            pos_y = 0;
            printf("点击了上一首\n");
            //先找到上一首mp3的文件节点
            tmp = last_song(tmp, my_list);
            printf("当前播放音乐文件名为:--->%s\n", tmp->Data.name);
            snprintf(str, 257, "madplay %s &", tmp->Data.name);
            //先清除上一次播放音乐的进程，防止重复播放
            system("killall -SIGKILL madplay");
            system(str);
        }

        //下一首
        if(pos_x > 465 && pos_x < 535 && pos_y > 405 && pos_y < 480){
            pos_x = 0;
            pos_y = 0;
            printf("点击了下一首\n");
            //先找到下一首mp3的文件节点
            tmp = next_song(tmp, my_list);
            printf("当前播放音乐文件名为:--->%s\n", tmp->Data.name);
            snprintf(str, 257, "madplay %s &", tmp->Data.name);
            //先清除上一次播放音乐的进程，防止重复播放
            system("killall -SIGKILL madplay");
            system(str);
        }

        //退出音乐播放器
        if(pos_x > 665 && pos_x < 735 && pos_y > 405 && pos_y < 480){
            pos_x = 0;
            pos_y = 0;
            printf("退出音乐播放器\n");
            system("killall -SIGKILL madplay");
            break;
        }
    }
}

//功  能:播放视频
void play_video(P_node my_list){
    //找到视频播放界面并且将播放视频状态机设置为真
    is_playing_video = true;
    P_node video_bmp = search_2_list(my_list, "video");
    P_node video = search_2_list(my_list, "111.avi");

    //播放标志位
    int flag = 0;
    char str[257];
    memset(str, 0, 257);
    display_node(video_bmp);
    while(1){
        //播放
        if(pos_x > 115 && pos_x < 190 && pos_y > 405 && pos_y < 480){
            pos_x = 0;
            pos_y = 0;
            if(flag == 0){
                printf("播放视频\n");
                snprintf(str, 257, "mplayer -x 800 -y 400 -Zoom %s &", video->Data.name);
                system(str);
            }
            else
                system("killall -SIGCONT mplayer");
            flag ++;
        }
        //暂停
        if(pos_x > 395 && pos_x < 470 && pos_y > 405 && pos_y < 480){
            pos_x = 0;
            pos_y = 0;
            printf("暂停播放...\n");
            system("killall -SIGSTOP mplayer");
        }
        //退出播放器且将播放视频状态机设置为假
        if(pos_x > 640 && pos_x < 710 && pos_y > 405 && pos_y < 480){
            pos_x = 0;
            pos_y = 0;
            printf("退出播放器...\n");
            is_playing_video = false;
            system("killall -SIGKILL mplayer");
            break;
        }
        /* if(pos_x > 640 && pos_x < 710 && pos_y > 405 && pos_y < 480){
            pos_x = 0;
            pos_y = 0;
            printf("点击了右键,快进10s!\n");
            fprintf(stdin,"%c",0x43);
        } */
    }
}

//功  能:读取天气预报文件
void read_weather_forecast(char  argv[][128]){
    FILE * fp = fopen(WEATHER_FILE, "r");
    char tmp[128];
    
    int i = 0;
    while(fgets(tmp, 128, fp) != NULL){
        //处理接收到的字符串的最后一个换行符，换成字符串结束符
        tmp[strlen(tmp)-1] = '\0';
        memcpy(argv[i++], tmp, 128);
    }
    fclose(fp);
}

//功  能:滚动刷新天气
void *flash_weather(void * arg){
    //阻塞1s等待刷新的开机画面
    sleep(1);
    char buf[6][128];
    memset(buf, 0, 6*128);
    read_weather_forecast(buf);
    for(int i = 0; i < 6; ){
        printf("\t---->%s\n", buf[i++]);
    }
    //打开字体
    font * f = fontLoad("/usr/share/fonts/simfang.ttf");
    //创建天气画板
    bitmap * bm_right= createBitmapWithInit(450,30,4,getColor(0,255,255,255));
    //设置字体大小
    fontSetSize(f, 24);
    //存储右边背景图的数据
    char back_right[450*30*4];
    memset(back_right, 0, 300*30*4);
    //获取首张画面的背景数据
    font_background( bm_right, 301, 0);
    memcpy(back_right, bm_right->map, 450*30*4);
    int start_pos = 450;
    while(1){
        //检测到播放视频时,阻塞显示
        if(is_playing_video){
            continue;
        }
        //检测到更换图片时，保存图片的背景数据
        if(change_bmp){
            change_bmp = false;
            //获取背景数据
            font_background( bm_right, 301, 0 );
            //将背景数据备份到备份内存内
            memcpy(back_right, bm_right->map, 450*30*4);
        }
        //将字体嵌入到画板中
        fontPrint(f, bm_right, start_pos, 0, buf[0], getColor(0, 34, 231, 255), 800);
        //将字体投映到lcd上
        show_font_to_lcd(map, 300, 0, bm_right);
        start_pos--;
        if(start_pos < -(3*24))
            start_pos = 450;
        usleep(100000);
        memcpy(bm_right->map, back_right, 450*30*4);
    }
}

//功  能:在状态栏刷新时间,以及滚动显示天气信息
void * flash_time(void * arg){
    //先等待刷新图片
    sleep(1);
   
    char buf[6][128];
    memset(buf, 0, 6*128);
    read_weather_forecast(buf);
    for(int i = 0; i < 6; ){
        printf("\t---->%s\n", buf[i++]);
    }
    //打开字体
    font * f = fontLoad("/usr/share/fonts/simfang.ttf");

    //创建时间画板
    bitmap * bm_left = createBitmapWithInit(300,30,4,getColor(0,255,255,255));
    //设置字体大小
    fontSetSize(f, 24);
    //存储左边背景图的数据
    char back_left[300*30*4];
    memset(back_left, 0, 300*30*4);
    font_background( bm_left, 0, 0 );
    memcpy(back_left, bm_left->map, 300*30*4);

    //创建天气画板
    bitmap * bm_right= createBitmapWithInit(450,30,4,getColor(0,255,255,255));
    //存储右边背景图的数据
    char back_right[450*30*4];
    memset(back_right, 0, 300*30*4);
    //获取首张画面的背景数据
    font_background( bm_right, 301, 0);
    memcpy(back_right, bm_right->map, 450*30*4);
    int start_pos = 450;
    int i = 0;
    while(1){
        //当检测到播放视频时，将继续循环，相当于阻塞状态
        if(is_playing_video){
            continue;
        }
        time_t timep; 
        time (&timep);
        char * p_time = ctime(&timep);
        //检测是否切换图片,如果是切换图片时，则保存图片的背景数据
        if(change_bmp){
            change_bmp = false;
            //设定延时，等待图片刷新完毕
            sleep(1);
            //获取背景数据
            font_background( bm_left, 0, 0 );
            //将背景数据备份到备份内存内
            memcpy(back_left, bm_left->map, 300*30*4);
            //获取背景数据
            font_background( bm_right, 301, 0 );
            //将背景数据备份到备份内存内
            memcpy(back_right, bm_right->map, 450*30*4);
            
        }

        //将时间信息写入到画板中
        fontPrint(f, bm_left, 0, 0, p_time, getColor(0, 34, 231, 255), 800);
        
        //将时间框输出到LCD屏幕上
        show_font_to_lcd(map, 0, 0, bm_left);
        // sleep(1);
        //睡1s之后利用背景图的数据覆盖文字实现清屏
        memcpy(bm_left->map, back_left, 300*30*4);
        //将天气嵌入到画板中
        fontPrint(f, bm_right, start_pos, 5, buf[i], getColor(0, 34, 231, 255), 800);
        //将天气投映到lcd上
        show_font_to_lcd(map, 300, 0, bm_right);
        start_pos--;
        if(start_pos < -(24*24)){
            start_pos = 450;
            i++;
        }
        if(i > 5)
            i = 0;
            
        usleep(100000);
        // sleep(1);
        memcpy(bm_right->map, back_right, 450*30*4);
    }
}

//改变字体的背景
void font_background(bitmap * bm, int x, int y){
    //提取背景的像素数据，填充到bm上
    unsigned int * p = ((unsigned int *)map + x + y*800);
    for(int i =  0; i < bm->height; i++){
        memcpy(bm->map+i*bm->width*4, p, bm->width*4);
        p += 800;
    }
}

//锁界面
bool project_lock(){
    
    int my_passwd[6] = {1, 2, 3, 4, 5, 6};
    int input_num[6] = {0, 0, 0, 0, 0, 0};
    volatile int i = 0;
    pos_x = 0;
    pos_y = 0;
flag_lock:
    i = 0;
    while( 1 ){
        // flash_time(NULL);

        //确认时退出
        if(pos_x > 225  && pos_x < 310 && pos_y > 310 && pos_y < 380 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0;
            break;
        }
        
        if(pos_x > 135  && pos_x < 215 && pos_y > 310 && pos_y < 380 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 0;
            printf("0\n");
            continue;
        }   //0
            
        else if(pos_x > 45   && pos_x < 130 && pos_y > 60 && pos_y < 140 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 1;
            printf("1\n");
            continue;
        }   //1
            
        else if(pos_x > 135  && pos_x < 215 && pos_y > 60 && pos_y < 140 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 2;
            printf("2\n");
            continue;
        }   //2
            
        else if(pos_x > 225  && pos_x < 310 && pos_y > 60 && pos_y < 140 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 3;
            printf("3\n"); 
            continue;
        }   //3
            
        else if(pos_x > 45   && pos_x < 130 && pos_y > 150 && pos_y < 220 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 4;
            printf("4\n");
            continue;
        }   //4
            
        else if(pos_x > 135  && pos_x < 215 && pos_y > 150 && pos_y < 220 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 5;
            printf("5\n");
            continue;
        }   //5
            
        else if(pos_x > 225  && pos_x < 310 && pos_y > 150 && pos_y < 220 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 6;
            printf("6\n");
            continue;
        }   //6
            
        else if(pos_x > 45   && pos_x < 130 && pos_y > 230  && pos_y < 300 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 7;
            printf("7\n");
            continue;
        }   //7
            
        else if(pos_x > 135  && pos_x < 215 && pos_y > 230  && pos_y < 300 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 8;
            printf("8\n");
            continue;
        }   //8
            
        else if(pos_x > 225  && pos_x < 310 && pos_y > 230  && pos_y < 300 && tc_state.end_pos.x){
            pos_x = 0;
            pos_y = 0; 
            input_num[i++] = 9;
            printf("9\n");
            continue;
        }   //9   

        //长按解锁
        if(pos_x > 470  && pos_x < 560 && pos_y > 330  && pos_y < 420){
            int now_x = pos_x;
            int now_y = pos_y;
            sleep(2);
            if(pos_x == now_x && pos_y == now_y)
                pos_x = 0;
                pos_y = 0;
                return true;
        }  
    }
        printf("当前输入密码是:\n");
        for( int j = 0; j < 6; j++)
            printf("%d",input_num[j]);
        printf("\n");
        int flag = 0;
        for(i = 0; i < 6; i++){
            if(input_num[i] != my_passwd[i]){
                printf("密码错误。。。请重新输入 \n");
                memset(input_num, 0, 6);
                flag = 1;
            }
        }
        if(!flag){
            return true;
        }else
            goto flag_lock;
}



//功  能:寻找上一首mp3
P_node last_song(P_node pos, P_node my_list){
    pos = list_entry(pos->ptr.prev, typeof(*pos), ptr);
    //跳过非mp3文件
    while(pos->Data.type != 'm'){
        pos = list_entry(pos->ptr.prev, typeof(*pos), ptr);
    }
    //跳过头节点
    if(pos == my_list){
        pos = list_entry(my_list->ptr.prev, typeof(*pos), ptr);
    }
    return pos;
}

//功  能:寻找下一首mp3
P_node next_song(P_node pos, P_node my_list){

    pos = list_entry(pos->ptr.next, typeof(*pos), ptr);
    //跳过非mp3文件
    while(pos->Data.type != 'm'){
        pos = list_entry(pos->ptr.next, typeof(*pos), ptr);
    }
    //跳过头节点
    if(pos == my_list){
        pos = list_entry(my_list->ptr.next, typeof(*pos), ptr);
    }
    return pos;
}

//功  能:获取键盘上下左右键
// void get_direction(){
    
// }

//功  能: 滑动更改图片显示(未完成)
void slide2change(unsigned int mode){
    switch (mode){
        case 1:
            for(int x = 0; x < 800; x++){
                for(int y = 0; y < 480; y++){
                    //将后面一列的数据往前覆盖
                    *(map + x + y*800) = *(map + x + (y+1)*800);
                }
            }
            // for(; varinfo.yoffset < 480; varinfo.yoffset++){
            //     ioctl(fd_lcd, FBIOPAN_DISPLAY, &varinfo);
            //     usleep(10000000);
            // }
            break;
        
        case 2:

            break;

        default:    break;
    }

}

