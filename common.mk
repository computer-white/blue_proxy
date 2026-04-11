# CC := g++
# BLUE := blue
# SIGNAL := signal
# TARGET := blue
# BLUE_CPP := $(wildcard $(BLUE)/*.cpp)
# HEADERS := $(wildcard $(BLUE)/*.h) # 查找头文件
# OBJECTS := $(patsubst $(BLUE)/%.cpp,%.o,$(BLUE_CPP)) # 将NGINX_C中的.c替换为.o
# # $< 第一个依赖,$^所有依赖,$@目标
# all : $(TARGET) # 默认命令
# %.o : $(BLUE)/%.c $(HEADERS)
# 	$(CC) -c $< -o $@ 

# $(TARGET) : $(OBJECTS)
# 	$(CC) $^ -o $@ 