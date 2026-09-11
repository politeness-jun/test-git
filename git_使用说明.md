
# 一、初始配置（仅首次）?

代码块?
git config --global user.name "用户名"?
git config --global user.email "邮箱"?
git config --global --list        # 查看配置?

## 二、仓库初始化?

代码块?
git init                         # 本地新建仓库?
git clone 仓库地址                # 克隆远程仓库?

## 三、日常开发核心流程?

代码块?
git status                       # 查看文件状态?
git add .                        # 全部文件加入暂存?
git commit -m "feat:新增功能"    # 提交本地?
git push origin 分支名           # 推送到远程?

## 四、拉取更新?

代码块?
git fetch origin                 # 拉取远程更新（不合并）?
git pull origin 分支名            # 拉取并合并远程代码?

