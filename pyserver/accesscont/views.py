from accesscont import app
from flask import jsonify, request
from accesscont import db
from accesscont.models import userinfo,attendancetime
import time
import datetime
from sqlalchemy import func
import math

from accesscont import audioplay

@app.route('/')
def index():
    return 'hello zjc233344444!'

# 定义键函数，返回字典中的 "name" 值
def get_name(item):
    return item["date"]

@app.route('/clockin', methods=["POST"])
def clockin():
    sth = request.json

    userid = sth['userid']
    # 先判断是否是已录入用户
    res1 = userinfo.query.filter(userinfo.userid==userid).first()
    if not res1:
        errmsg = "非法用户，无权进入！"
        # 调用语音函数
        audioplay.generateaudio(errmsg,1)
        audioplay.play_mp3(1)
        return jsonify({"msg":errmsg,"tag":0})
    else:
        res2 = attendancetime.query.filter(attendancetime.userid == userid,attendancetime.status==0).first()
        # 找不到已上班打卡信息，则记录打卡
        if not res2:
            totime = int(time.time())
            current_timestamp = time.time()
            current_struct_time = time.localtime(current_timestamp)
            year = str(current_struct_time.tm_year)
            month = str(current_struct_time.tm_mon)
            day = str(current_struct_time.tm_mday)
            m1 = attendancetime(username=res1.username,userid=res1.userid,status=0, toworktimestamp=totime,createddate=year+'-'+month+'-'+day)
            db.session.add(m1)
            db.session.commit()
            toworkmsg = "上班打卡成功！"+"打卡人："+res1.username+"员工ID为："+str(userid)
            audioplay.generateaudio(toworkmsg,2)
            audioplay.play_mp3(2)
            return jsonify({"msg":"上班打卡成功！","tag":1,"user":res1.username,"userid":res1.userid})
        # 存在上班打卡信息，则记录下班信息
        else:
            res2.status = 1
            res2.offworktimestamp = int(time.time())
            db.session.commit()
            # 统计本次出勤时间
            toworktime = int(res2.toworktimestamp)
            attendtime = res2.offworktimestamp-toworktime

            # 统计一个自然月出勤时间
            current_year = datetime.date.today().year
            current_month = datetime.date.today().month
            start_date = datetime.date(current_year, current_month, 1)
            end_date = start_date + datetime.timedelta(days=31)
            results = attendancetime.query.filter(
                func.extract('year', attendancetime.createddate) == current_year,
                func.extract('month', attendancetime.createddate) == current_month,
                attendancetime.createddate >= start_date,
                attendancetime.createddate < end_date,
                userinfo.userid == userid
            ).all()
            # 上面统计本人本月出勤记录
            daytotaltime = 0
            resultsnew = []
            analyres = []
            for x in results:
                newdict = { "date":x.createddate,"clockintime":x.offworktimestamp-x.toworktimestamp}
                # print("date:",x.createddate," toworktime",x.toworktimestamp," offworktime",x.offworktimestamp)
                resultsnew.append(newdict)
            # 统计每次出勤时间
            attendtimetagarray = list({get_name(item): item for item in resultsnew}.values())
            # 去重，生成日期数组
            for x in attendtimetagarray:
                for y in resultsnew:
                    if x['date']==y['date']:
                        daytotaltime+=y["clockintime"]
                date_number = int(x['date'].strftime("%Y%m%d"))
                newdict = {date_number:daytotaltime}
                analyres.append(newdict)
                daytotaltime=0
            # 统计每天出勤时间，生成日期对于时间的数组
            monthtotaltime = 0
            for x in analyres:
                values = x.values()
                for y in values:
                    monthtotaltime+=y
            # 统计每个自然月总的出勤时间
            timelens = 1
            fakemonthtotaltime = monthtotaltime
            while(fakemonthtotaltime>9):
                fakemonthtotaltime=fakemonthtotaltime/10
                timelens = timelens+1
            actualvalue = int(timelens*math.pow(10,timelens)+monthtotaltime)
            # 设计第一位为总出勤时间的位数，第一位后为实际出勤时间
            # for x in analyres:
            #     print("date:",x)
            hour = int(attendtime/3600)
            minute = int((attendtime%3600)/60)
            second = int((attendtime%3600)%60)
            offworkmsg = "下班打卡成功！"+"本次出勤时间为："+str(hour)+"小时，"+str(minute)+"分钟，"+str(second)+"秒。"
            audioplay.generateaudio(offworkmsg,3)
            audioplay.play_mp3(3)
            # 调用语音函数，进行播报
            return jsonify({"msg": "下班打卡成功！","attendtime":attendtime,"tag":2,"monthclockintime":actualvalue,"user":res1.username,"userid":res1.userid})
