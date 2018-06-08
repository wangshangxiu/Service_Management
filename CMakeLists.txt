# Exchange DEPT.
# Savin Chen
cmake_minimum_required(VERSION 2.8)
project(ManagerSrv)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11  -DKEEP_ALIVE_TEST -DDEBUG  -DLOCALHOSTIP ")

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/tnode2.0.1/include)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/xlogger/include)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/db/include)

set(SOURCE_FILE 
	src/Main.cpp
	src/Public.cpp
	src/ConfigFileReader.cpp
	src/MemPoolMgr.cpp
	src/MgrConsumer.cpp
	src/MgrBase.cpp
	src/LoginOutMgr.cpp
	src/AssetsMgr.cpp
	src/UserMgr.cpp
	)


link_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/tnode2.0.1/lib)
link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/xlogger/lib)
link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/db/lib)
link_directories(/usr/local/lib/mysql)

add_executable(bin/ManagerSrv ${SOURCE_FILE})


target_link_libraries(bin/ManagerSrv tnode2 rabbitmq xlogger glog pthread redisop hiredis  mysqlop mysqlclient) 

