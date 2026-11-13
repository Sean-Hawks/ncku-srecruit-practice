# ============================================================================
#  考前練習材料 -- 兩個在自己電腦上就能跑的小程式
#
#      make                      編譯
#      make run                  依序執行兩個 demo
#      make run-critical-section 只跑 critical section
#      make run-transfer-engine  只跑 transfer engine
#      make mistake              transfer engine 的「最常見錯誤」示範
#      make clean                清掉編譯產物
#      make help                 顯示這份說明
# ============================================================================
# make 是讀這份規則、決定要執行哪些命令的工具；真正編譯的是 cc。
# ?= 表示尚未指定才使用預設值；變數可從 make 命令列覆寫。
CC     ?= cc
# -O2 做最佳化；-Wall/-Wextra 開警告；-pthread 提供執行緒所需編譯/連結設定。
CFLAGS ?= -O2 -Wall -Wextra -pthread
# 執行檔輸出目錄。src 的 .c 仍留在原處，編好的程式放 bin/。
BIN    := bin

CS := $(BIN)/critical-section
TE := $(BIN)/transfer-engine

# 這些是操作名稱，不拿同名檔案的時間戳記判斷是否完成。
.PHONY: all run run-critical-section run-transfer-engine mistake clean help

# 單獨 make：先建兩支執行檔，尚未執行它們。第一個一般目標是預設目標。
all: $(CS) $(TE)
	@echo
	@echo "編譯完成。接著試試：  make run"

$(BIN):
	@mkdir -p $(BIN)

# 目標: 依賴。原始碼更新或執行檔不存在，就重編譯。
# | $(BIN) 只要求先有 bin 目錄；不因目錄時間變動而每次重編譯。
$(CS): critical-section/main.c | $(BIN)
# $@ 是目前目標路徑；$< 是第一個依賴檔。下面命令行開頭必須是 Tab。
	$(CC) $(CFLAGS) -o $@ $<

# 你問 engine.h、engine.c 有沒有用到：有，這裡三份檔案都是重建依據。
# main.c 和 engine.c 各自 include engine.h，讀取型別和函式宣告。
# 下方 cc 同時編譯兩份 .c，再連結成一支 bin/transfer-engine；
# .h 不用另外當成一支程式編譯。任一 .c 或 .h 更新，都會重新建置。
$(TE): transfer-engine/main.c transfer-engine/engine.c transfer-engine/engine.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ transfer-engine/main.c transfer-engine/engine.c

# make run：依這兩個目標執行兩份 demo。
run: run-critical-section run-transfer-engine

run-critical-section: $(CS)
	@echo "=============== critical section ==============="
	@./$(CS)
	@echo

# make run-transfer-engine：先確保執行檔是新的，再執行它。
# cc 那一長串是 make 根據規則幫你組出的，不必背。
run-transfer-engine: $(TE)
	@echo "=============== transfer engine ================"
	@./$(TE)
	@echo

# make mistake：同一支執行檔，加上 --mistake 參數。
# main(argc,argv) 讀到它後，才追加故意填錯 count 的第 3 個示範。
mistake: $(TE)
	@echo "=============== 最常見的錯誤 ==================="
	@./$(TE) --mistake
	@echo

# 清除 bin 裡的編譯產物；不刪原始碼。下次 make 會重新編譯。
clean:
	rm -rf $(BIN)

# @ 表示不印出這條命令本身；sed 只是印出檔案開頭的操作清單。
help:
	@sed -n '2,10p' Makefile | sed 's/^# \?//'
