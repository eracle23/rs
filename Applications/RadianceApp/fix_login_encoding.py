# -*- coding: utf-8 -*-
path = r'e:\GitHub\rs0\rs\Applications\RadianceApp\Widgets\qLoginDialog.cxx'
with open(path, 'r', encoding='utf-8') as f:
    c = f.read()

# Replace mojibake (UTF-8 read as Latin-1) with correct Chinese
replacements = [
    (r'q->setWindowTitle(QString::fromUtf8("ç¨æ·ç­å½"));', 'q->setWindowTitle(QString::fromUtf8("用户登录"));'),
    ('TitleLabel = new QLabel(QString::fromUtf8("??????????"));', 'TitleLabel = new QLabel(QString::fromUtf8("医学影像三维重建软件"));'),
    ('SubtitleLabel = new QLabel(QString::fromUtf8("??????????"));', 'SubtitleLabel = new QLabel(QString::fromUtf8("请登录以继续使用软件"));'),
    ('UsernameLabel = new QLabel(QString::fromUtf8("????"));', 'UsernameLabel = new QLabel(QString::fromUtf8("用户名："));'),
    ('UsernameEdit->setPlaceholderText(QString::fromUtf8("??????"));', 'UsernameEdit->setPlaceholderText(QString::fromUtf8("请输入用户名"));'),
    ('PasswordLabel = new QLabel(QString::fromUtf8("???"));', 'PasswordLabel = new QLabel(QString::fromUtf8("密码："));'),
    ('PasswordEdit->setPlaceholderText(QString::fromUtf8("?????"));', 'PasswordEdit->setPlaceholderText(QString::fromUtf8("请输入密码"));'),
    ('LoginButton = new QPushButton(QString::fromUtf8("??"));', 'LoginButton = new QPushButton(QString::fromUtf8("登录"));'),
    ('ForgotPasswordButton = new QPushButton(QString::fromUtf8("?????"));', 'ForgotPasswordButton = new QPushButton(QString::fromUtf8("忘记密码？"));'),
    ('ExitButton = new QPushButton(QString::fromUtf8("??"));', 'ExitButton = new QPushButton(QString::fromUtf8("退出"));'),
    ('QMessageBox::warning(this, QString::fromUtf8("????"),\n                           QString::fromUtf8("??????"));', 'QMessageBox::warning(this, QString::fromUtf8("登录失败"),\n                           QString::fromUtf8("请输入用户名"));'),
    ('QMessageBox::warning(this, QString::fromUtf8("????"),\n                           QString::fromUtf8("?????"));', 'QMessageBox::warning(this, QString::fromUtf8("登录失败"),\n                           QString::fromUtf8("请输入密码"));'),
    ('d->LoginButton->setText(QString::fromUtf8("???..."));', 'd->LoginButton->setText(QString::fromUtf8("登录中..."));'),
    ('QString message = QString::fromUtf8("?????\\n\\n?????%1")', 'QString message = QString::fromUtf8("登录成功！\\n\\n欢迎回来，%1")'),
    ('QMessageBox::information(this, QString::fromUtf8("????"), message);', 'QMessageBox::information(this, QString::fromUtf8("登录成功"), message);'),
    ('QMessageBox::critical(this, QString::fromUtf8("????"), error);', 'QMessageBox::critical(this, QString::fromUtf8("登录失败"), error);'),
    ('d->LoginButton->setText(QString::fromUtf8("??"));', 'd->LoginButton->setText(QString::fromUtf8("登录"));'),
    ('QMessageBox::information(this, QString::fromUtf8("????"),\n                       QString::fromUtf8("???????????\\n\\n??????admin@radiancelabs.com"));', 'QMessageBox::information(this, QString::fromUtf8("忘记密码"),\n                       QString::fromUtf8("请联系管理员重置密码。\\n\\n管理员邮箱：admin@radiancelabs.com"));'),
]

for old, new in replacements:
    c = c.replace(old, new)

with open(path, 'w', encoding='utf-8') as f:
    f.write(c)
print('Done')
