from accesscont import db

# user info
class userinfo(db.Model):
    id=db.Column(db.Integer,primary_key=True,nullable=False,autoincrement=True)
    username=db.Column(db.String(30))
    userid=db.Column(db.Integer)

# 考勤时间表
class attendancetime(db.Model):
    aid = db.Column(db.Integer, primary_key=True, nullable=False, autoincrement=True)
    username = db.Column(db.String(30))
    userid = db.Column(db.Integer)
    # 两种状态，为0上班打卡，为1表示下班打卡
    status = db.Column(db.Integer)
    toworktimestamp = db.Column(db.Integer)
    offworktimestamp = db.Column(db.Integer)
    # 设定日期，便于统计一个自然月内信息
    createddate = db.Column(db.Date)

