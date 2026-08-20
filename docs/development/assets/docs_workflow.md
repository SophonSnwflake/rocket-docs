# **文档开发规范**

文档AI等级：2

本文档开发规范用于规定两个方面：文档的修改流程和文档写作格式上的规范

---

## **1 从零开始的文档修改流程**

首先认识一下这个文档的基本原理

本文档采用的是mkdocs + github网站托管形式，mkdocs是一个静态网站生成器，mkdocs会将我们写的.md文档“翻译”成HTML文件，而github则会充当一个服务器的功能，将mkdocs生成的HTML文件时时托管在云端。本网页的后台仓库所存储的所有后缀名为.md的文件就是我们修改文档时所要更改的对象，它既是网页的显示形式，也可以独立出来看成是相互关联的文档。

### **1.1 安装mkdocs**

我们目前所看到的网页是github每次收到推送时自己调用的mkdocs来编译的网站文件，但是我们在写.md的文档的时候时常需要自己调用mkdocs来启动服务器来预览效果，下面展示安装流程。
本流程参考官方文档，一切细节请以官方文档为参考：[MkDocs](https://hellowac.github.io/mkdocs-docs-zh/)

### **1.1.1 安装python**

推荐使用包管理器直接下载python
在非中文工作路径下打开终端并执行以下命令：

```powershell
winget install Python.Python.3.13
```

待进度条结束后执行：

```powershell
python --version
```

如能成功报出安装版本即为成功。

### **1.1.2 使用pip获取Mkdocs**

更新pip：

```powershell
pip install --upgrade pip
```

使用pip安装MkDocs：

```powershell
pip install mkdocs
```

运行mkdocs --version来检查一切是否正常

```powershell
$ mkdocs --version
mkdocs, version 0.15.3
```

好了，现在你的电脑python环境内已经有了一个全局的mkDocs了

### **1.3 工作流详细步骤**

假设你的电脑上已装有git，如果没有装请在终端内运行

```powershell
winget install --id Git.Git -e

git --version # 确认是否git成功安装

git config --global user.name "你的名字"  # 绑定git名称
git config --global user.email "你的GitHub邮箱" # 绑定git邮箱
```

保证你有一个github账号，如果没有请去github官网注册：[github](https://github.com/)

### **1.3.1 创建fork仓库**

fork仓库是原始仓库的副本，所有开发工作均在fork仓库中进行，完成后通过pull request将代码合并回原始仓库。

你对fork仓库拥有完全的读写权限，fork仓库不会影响原始仓库的代码（pull request前），其在保护了原始仓库的完整性的同时，允许你自由地进行开发和实验。

访问原始仓库：[原始仓库](https://github.com/SophonSnwflake/rocket-avionics-docs)，点击右上角的*Fork*按钮，将仓库fork到自己的个人仓库。

### **1.3.2 克隆Fork仓库到本地**

执行：
```powershell
git clone <你的fork仓库地址>
```

确认仓库分支指向，运行：

```powershell
git status
```

如果看到类似：

```powershell
On branch main
nothing to commit, working tree clean
```

说明仓库没问题，目前指向main分支

### **1.3.3 添加上游仓库**

为了使得fork仓库的更改能够通过Pull-Request传递到原始仓库，我们需要在你fork的仓库下将原始仓库添加到远程仓库地址

通过一下命令可以看到当前fork仓库的远程仓库地址：

```powershell
git remote -v
```

输出：

```powershell
$ git remote -v
origin https://github.com/xxx/rocket-avionics-docs.git (fetch)
origin https://github.com/xxx/rocket-avionics-docs.git (push)
```

可以看到当前的远程仓库所关联的“orgin”都是你自己刚刚克隆的自己的fork仓库。所以我们需要在此基础上再添加一个原始仓库的远程地址，命名为upstream，运行：

```powershell
git remote add upstream https://github.com/SophonSnwflake/rocket-avionics-docs
```

可以再次执行git remote -v，输出应该包含两个origin（自己的fork）和两个upstream（原作者的原始仓库）地址。

一个典型的基于fork仓库的开发环境中一共存在以下三个仓库：
- 本地仓库(local)：存储在你电脑上的仓库
- 远程fork仓库(origin)：存储在你的GitHub账号下的fork仓库
- 远程上游仓库(upstream)：存储在原作者账号下的原始仓库

### **1.3.4 创建用于本次更改的分支**

永远不要直接在main分支上开发，必须新建分支。

（可选）确保在main分支上并习惯性的同步一下上游最新代码：

```powershell
git switch main
git pull upstream main
```

创建并切换到新分支：

```powershell
git switch --create <新分支名>
```

新分支命需简短的描述一下你本次更改的内容，比如：增添电控板块的某某文档

### **1.3.5 开发和提交代码**

在新建的分支上进行代码开发，以下是常用Git命令：

```powershell
# 暂存所有修改的文件
git add .

# 提交代码，-m后跟提交信息
git commit -m "简要描述本次提交的内容"
# 或使用等效长参数 --message：
git commit --message "简要描述本次提交的内容"

# 推送代码到远程fork仓库(origin)
git push origin <your-branch-name>
# 或简写(若已通过git push -u设置好上游分支):
git push
```

当你修改了某个文档，想在自己的电脑上看看渲染效果的时候，可以执行：

```powershell
# 创建一个服务
mkdocs serve
```

然后将弹出的链接复制进浏览器，一般是 `http://127.0.0.1:8000/`

即可看到当前的网站效果


### **1.3.6 合并前准备：同步上游与整理提交**
在提交Pull Request(PR)前，再次确保代码是基于上游最新代码开发的，并整理提交记录。假设在你开发的过程中，上游原始仓库已经有了其他人的新的提交/合并
拉取上游main分支(upstream/main)的最新代码到本地main分支(main)：

```powershell
# 切换到本地main分支
git switch main
# 拉取上游最新代码到本地main分支
git pull upstream main
```

顺手更新一下远程fork仓库的main分支(origin/main)：

```powershell
git push origin main
# 或简写(若已通过git push -u设置好上游分支):
git push
```

接下来变基并整理提交历史：

```powershell
# 切换回你的开发分支
git switch <your-branch-name>
# 交互式变基到最新的main分支
git rebase -i main
```

此时会打开一个文本编辑器(git默认文本编辑器)，列出你在该分支上的所有提交记录。按照提示修改提交记录，例如合并多个零碎提交为一个有意义的提交，修改提交信息等。比如，将多个小的修复提交的pick改为squash，将琐碎的“fix typo”合并成一个大的Commit。

```markdown
提示：如果你没有配置过git的默认文本编辑器，git大概率会使用Vim作为文本编辑器，建议将git默认文本编辑器配置为你熟悉的编辑器，如VSCode，如果已经不慎进入了Vim编辑器，以下是一些基本操作：
• 按下 i 键进入编辑模式，此时左下角显示 -- INSERT -- ，使用方向键移动光标，输入
文本进行编辑
• 编辑完成后，按下 Esc 键退出编辑模式
• 依次（不是同时）按下键盘上的:(大写冒号)w q Enter 来保存并退出Vim编辑器
```

若在交互式变基过程中遇到冲突，git会提示你哪些文件存在冲突，请手动打开这些文件，解决冲突并保存文件后，执行以下命令继续变基：

```powershell
# 暂存解决冲突后的文件(<conflicted-file>替换为实际冲突的文件名)
git add <conflicted-file>
# 继续变基
git rebase --continue
```

完成变基后，使用带保护的强制推送将你的开发分支推送到远程fork仓库(origin)：

```powershell
git push --force-with-lease origin <your-branch-name>
# 或简写(若已通过git push -u设置好上游分支):
git push --force-with-lease
```

### **1.3.7 创建Pull Request请求合并分支**

在GitHub上打开你的fork仓库页面，切换到你刚刚推送的分支 <your-branch-name> ，点击`Compare & pull request`按钮，填写PR标题和描述后，点击`Create pull request`按钮提交PR请求。

### **1.1.8 代码评审与收尾工作**

PR创建后，等待代码评审人员的审核和反馈，若有修改建议，请在本地分支进行修改并提交，再重新推送到远程fork仓库(origin)，PR会自动更新。具体步骤为：

```powershell
# 切换到你的分支
git switch <your-branch-name>
# 根据反馈意见在本地分支进行修改
# 暂存修改
git add .
# 提交修改
git commit -m "根据反馈意见修改后的提交信息"
# 推送修改到远程fork仓库(origin)
git push origin <your-branch-name>
# 或简写(若已通过git push -u设置好上游分支):
git push
```

PR审核通过后，恭喜你，你的代码被成功合并到上游仓库的主分支中了，接下来可以清理(可选)本地工作现场以准备下一个开发任务：

```powershell
# 切换回main分支
git switch main
# 拉取上游最新代码到本地main分支
git pull upstream main
# 顺手更新一下远程fork仓库的main分支(origin/main)
git push origin main
# 或简写(若已通过git push -u设置好上游分支):
git push
# 删除远程fork仓库(origin)的分支(可选，若确认该分支不再使用)
git push origin --delete <your-branch-name>
# 更新本地仓库，并裁剪无用的远程分支引用
git pull --prune
# 删除本地开发分支(可选，若确认该分支不再使用)
git branch --delete <your-branch-name>
# 或简写:
git branch -d <your-branch-name>
```

