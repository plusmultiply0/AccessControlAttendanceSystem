import click
from accesscont import app,db
from accesscont.models import userinfo,attendancetime

#python shell上下文
@app.shell_context_processor
def make_shell_context():
    return dict(db=db,userinfo=userinfo,attendancetime=attendancetime)

# 初始化数据库，建表
@app.cli.command()
@click.option('--drop', is_flag=True, help='Create after drop.')
def initdb(drop):
    """Initialize the database."""
    if drop:
        click.confirm('This operation will delete the database, do you want to continue?', abort=True)
        db.drop_all()
        click.echo('Drop tables.')
    db.create_all()
    click.echo('Initialized database.')

# 往表中插入初始数据
@app.cli.command()
def build():
    """Generate prepared messages."""
    click.echo('Working...')
    # 预先录入员工信息
    m = userinfo(username='zjc',userid=1)
    db.session.add(m)
    m = userinfo(username='张三', userid=3)
    db.session.add(m)
    # 预先录入考勤信息
    m = attendancetime(username='zjc',status=1,userid=1,toworktimestamp=1686186000,offworktimestamp=1686196800,createddate='2023-06-08')
    db.session.add(m)
    m = attendancetime(username='zjc', status=1,userid=1,toworktimestamp=1686204000, offworktimestamp=1686216600,
                       createddate='2023-06-08')
    db.session.add(m)
    db.session.commit()
    click.echo('ok!')
