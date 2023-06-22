# 门禁考勤系统

## 概述

本系统基于C++和Python语言进行开发，客户端（接读卡器部分）由C++实现，服务器端由Python的Flask框架实现。

## 基本功能

- 预先录入员工有关信息，并进行存储
- 当识别到有卡时，识别录入员工
  - 打卡时，若无上班打卡记录，录入上班时间并语音播报
  - 打卡时，若有上班打卡记录，读取下班时间以及统计出勤时间并语音播报
  - 识别非录入员工，显示“无权进入”并语音提示
- 一天可多次上下班，并能统计有效出勤时间。
- 语音播报上下班打卡信息和非法用户信息

## 如何使用本项目

### 服务器端部分

1. 使用`git clone`下载项目代码
2. 进入pyserver文件夹，在 python3 环境下，创建python虚拟环境 rfidvenv`python -m venv rfidvenv`，激活虚拟环境 `rfidvenv\scripts\activate`
3. 在虚拟环境下，安装依赖库 `pip install -r requirements.txt`
4. 在pyserver目录里，创建一个 .env 文件，并写入如下代码。将root替换为你的 MySQL 的登录名，123456替换为登录密码。然后在你的本地MySQL里创建一个名为dbtest的数据库DB。在讯飞开放平台注册账号，创建一个语音合成应用，在**服务接口认证信息**处，将xxx替换为对应的信息。

```
DATABASE_URL=mysql+pymysql://root:123456@localhost:3306/dbtest
APPID=xxx
APISecret=xxx
APIKey=xxx
```

5. 在前面的虚拟环境中，先运行`flask initdb` 初始化数据库，再运行`flask build` 填充预置数据，最后运行`flask run --host=0.0.0.0`，在 http://127.0.0.1:5000/ 查看项目。

### 客户端部分

1. 使用clion打开clionproject
2. 电脑插上虚拟串口USB转232和13.56M的读卡器，点击运行

## 注意事项

- 需要先运行服务器端再运行客户端代码
- 运行时，需要修改curl访问服务器的地址，改成相应的运行地址或内网地址
- 读卡器需要设置为命令模式
- 默认读取卡的14扇区56块作为卡主信息
- 默认将自然月内打卡时间写入卡的15扇区60块
- 自然月内打卡时间设计为数据（实际出勤的秒数）的位数+实际出勤的秒数
- （两台电脑的情况下）推荐使用性能较差的电脑作为服务器端，因为服务器端代码相比客户端更容易配置

## 参考资料

- https://github.com/plusmultiply0/bookmanagesystem
- https://www.xfyun.cn/doc/tts/online_tts/API.html#%E6%8E%A5%E5%8F%A3%E8%B0%83%E7%94%A8%E6%B5%81%E7%A8%8B