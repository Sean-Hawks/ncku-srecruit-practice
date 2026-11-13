# Git 與提交：知道自己在哪個 repo、改了什麼

**Git commit 存在本機；push 才傳上遠端；pull 會抓遠端並整合到本機。** 不要在不確定的資料夾一直 git init。

## 拉這份筆記：在主機

目前正確 repo：

```bash
cd ~/Development/ncku-srecruit-practice
git status --short --branch
git branch -vv
git remote -v
```

沒有本機修改、目前在 main 時：

```bash
git pull --ff-only origin main
```

`--ff-only` 不會自動建立 merge commit；若兩邊各有 commit，會停止，先看差異再處理。檔案在 nvim 開著時用 `:checktime` 重新檢查；`:e!` 會丟未存修改，別拿來當一般更新。

## 你遇過：There is no tracking information

意思：本機目前分支沒設定「預設跟誰同步」。先 `git branch --show-current`，確認是要用來讀筆記的 main，再：

```bash
git fetch origin
git branch --set-upstream-to=origin/main main
git pull --ff-only
```

上面只適合本機 main 確實要追 origin/main。其他分支不要一律接到 main；也可以單次明確指定 `git pull --ff-only origin main`，但仍須知道自己目前在哪個分支。

如果顯示不是 Git repo，先切到正確資料夾；首次取得才 clone：

```bash
cd ~/Development
git clone git@github.com:Sean-Hawks/ncku-srecruit-practice.git
```

私人 repo 需要帳號權限與可用的 SSH 設定。

## 修改與提交：先看，再選檔

在**實際作答 repo 根目錄**：

```bash
git status --short --branch
git diff
git diff --check
```

確認這次修改的檔案後：

```bash
git add <這次要交的檔案>
git diff --cached
git commit -m "<依現場規則填提交訊息>"
git push
```

先替換 `<...>`。不預設題號前綴、分支名或 push 次數，**全部以當年題目與 GitLab 說明為準**。不要把別年的 commit message 規則當今年規則。

有現場 CI 時：push 完看 pipeline／評分結果；本機 commit 成功不代表遠端已收到，也不代表評分通過。

## 有修改，pull 不過

先 `git diff` 看是你的練習還是正式作答。可選擇把確定要保留的工作 commit；或暫存到 stash：

```bash
git stash push -u -m "before updating notes"
git pull --ff-only
git stash pop
```

stash pop 可能衝突，需逐段保留正確內容。不是執行完就一定解決；不要用 reset --hard 或 force push 掩蓋問題。

來源：[git pull](https://git-scm.com/docs/git-pull)、[git branch](https://git-scm.com/docs/git-branch)、[git stash](https://git-scm.com/docs/git-stash)。

[回技術目錄](README.md)
