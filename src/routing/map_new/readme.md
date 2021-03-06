##环境依赖：  
1.python 3.5 及以上  
2.numpy 1.16.2  
3.matplotlib 3.0.3  
4.scipy 1.2.1  
5.dxfgrabber  
6.tkinter  
7.networkx  
8.socketserver  
9.threading  
10.time  

##数据依赖：  
1.data目录下的dxf文件  

##操作流程：  
1.'文件' -> '选择路径'：  

2.鼠标左键点：选中车道参考线  
3.鼠标左键点：选中车道参考线起点  
4.鼠标右键点：弹出对话框，配置车道参考线属性  
5.配置完成，点击‘确认’退出  

6.鼠标左键点：选中车道参考线一侧的一段车道边界（可以多条线）  
7.鼠标右键点：弹出对话框，配置车道边界  
8.配置完成，点击‘确认’退出  
9.重复6、8，直到当前选中车道参考线的车道边界完成  

10.点击鼠标右键，弹出对话框，确认参考线配置，点击‘确认’退出  

11.重复2-10，直到全部车道参考线配置完成，点击‘设定’-》‘处理附属物’  

12.鼠标左键点：选中一个车道附属物（可以多条线）  
13.鼠标右键点：弹出对话框，配置附属物属性  
14.鼠标左键点：选中一个车道参考线（该车道附属物将成为该车道参考线的附属）  
15.鼠标右键点：弹出对话框，点击‘确认’退出  
16.重复12-15  

17.点击‘文件’-》‘生成连通图’  

18.点击‘x’，退出  

##预期升级特性：  
1.增加功能：修改车道参考线属性（即不需要再重新选择车道边界）  
2.增加功能：修改附属物属性  
3.增加功能：直接在python界面添加各种附属物，动态增加临时障碍物  
4.增加功能：从连通图删除车道参考线  
5.增加功能：各个弹出对话框的‘取消’功能  
6.增加功能：附属物归属多个ref_line  
7.增加功能：在ref_line上增加车道属性  

##地图文件说明：  有符号数
appendix_attribute.json  
    1.自身id  
    2.归属于哪条ref_line  
    3.类型：参见 attribute_def.py 文件中 APPENDIX_ATTRIBUTE_VALUE(1/2/3...)  
    4.位置坐标数量  
    5.位置坐标序列(n个GPS坐标)，每个GPS包括x,y   

ref_points.json  
    0. ref_line_id：  
    1. GPSpoint：x, y, z  
    2. 车道数量：1、2、3
    3. width,例如 [-6,-2][-2,2] 左->右,参考线点左负右正 
    4. cuv：1/米  
    5. gcuv：1/米^2  
    6. s: 米  
    7. theta：弧度（0-2pi）  

ref_line.json  
    1.id  
    2.限速,[低，高]  默认[1,10] m/s
    3.限高, 默认20m
    4.曲率, 根据dxf获取 1/m, 直线为0, 圆弧为1/radius
    5.车道数量  
    6.坡度: 暂时没用  
    7.行车方向：中断0x00 / 单向0x01 / 左转0x02 / 右转0x04 / 双向0x08   
    8.参考线长度  
    9.参考线起点: x, y, z  
    10.参考线终点  
    11.材质  
    12.区域属性: 参见 attribute_def.py 文件中 ROAD_AREA_VALUE (1/2/3...)  

ref_line_appendix.json  
    1.ref_line_id  
    2.appendix_id:可以多个  

connect_map.json  
    1.start_ref_line_id  
    2.next_ref_line_id: 可以多个  

area.json (2019-09-29)  
    第一行:xmin,xmax；  
    第二行:ymin,ymax  

当前位置判断道路id时需要判断道路方向

# routing.py 说明
1.因为ref_line中增加了限高、转弯半径等参数，因此现在可以根据发来的车辆ID和属性（最大高度/转弯半径）判断车辆是否可以通过某个车道，从而建立临时的连通图

# map_tools使用说明  20190819
## 依赖  
python 3.5 以上  
dxfgrabber   
matplotlib   
numpy  
enum  
tkinter

## 使用流程
0.首先需要使用cad文件在不同的图层上描画出地图参考线(ref)/车道边界(board)/车道附属物(appendix)   
1.在map_tools.py所在目录下,运行 python map_tools.py   
2.稍等片刻,等待python载入所需库   
3.在主界面上,点击"文件"->"选择DXF文件"   
4.在弹出窗口选中计划打开的dxf文件   
5.之后弹出一个窗口,分别表示 ref/board/ref 所在的图层,根据cad的实际绘制情况,勾选对应图层   
6.点击确认   
7.在主界面左侧,会显示当前载入的DXF文件,颜色与dxf中的图层设置一致,如果某个地图元素(ref/appendix)曾经被配置过,则会显示为黑色   
8.鼠标左键点击ref/appendix,主界面右侧会显示相应的属性   
9.如果是处理ref,则需要在右侧属性中填写和选择各个选型   
10.在处理ref时,需要选择ref的起点,此时需要在已经选中的ref上,靠近起点处单击鼠标左键   
11.如果起点选择错误,则在正确的起点处单击鼠标左键即可以重新选择   
12.在处理ref时,需要选择ref对应的车道边界,鼠标左键在对应的车道边界上单击即可,一次可以选择多条边界   
13.在处理ref时,如果起点/边界都已经选择完成,则可以点击ok按键,会生成参考线离散点并保存,同时ref会置黑   
14.如果出现问题,会弹出提示对话框,点击确定后根据提示进行处理(目前不能处理车道分隔物为大面积物体,此时必须当作两个车道处理)   15.如果处理appendix(目前不能处理红绿灯),允许通过鼠标左键单击同时选中多个appendix,但要求其属性和所属ref一致,在对应appendix上右键单击可以取消选择   
16.处理appendix时,选择其所属ref,只需要鼠标左键在对应ref上单击即可,在对应ref上右键单击可以取消选择  
17.完成appendix处理后,点击ok,会保存数据,同时配置过的appendix会置黑  
18.处理完成后,在主界面主菜单上,可以选择"文件"->"生成连通图",会生成并保存连通图文件  
19.如果需要检测连通图,可以在主菜单上选择"校验"->"检查连通图"  
20.当检查连通图时,通过点击弹出界面的"上一条","下一条",可以在主界面看到连通关系(绿色为起始段,红色为下一段)  
21.当发现下一段的连通关系不对时,可以点选弹出界面中下一段对应的id(图中会通过变黑来显示),从而确认下一段是否与当前段连接(目前不能增加连接段,只能减少)  
22.当确认完成,可以单击弹出界面中的ok键退出  
23.如果发现连通关系更改有误,可以单击cancle退出,也可以点击"上一条"/"下一条"到对应的参考线路段,重新选择  
24.如果发现连通关系修改有误,且已经单击ok退出,那么可以重新执行上述中第18步,重新生成连通图  
25.该工具允许中途退出,已经完成配置的地图元素数据都将得到保存

## 注意事项
0.车道附属物只能使用PLINE或者POINT,不能使用line/arc等方法  
1.一旦车道附属物/参考线在CAD工具中重绘过,不论是拖动,还是修改属性,都必须删除dxf文件目录下的所有json文件,防止出现错误  
2.ref和board在每个交叉点上都必须是截断并分割开的  
3.appendix中的停止线目前必须画成矩形框,和人行横道保持一致.  

# 新版本记录
#### 2019.10.23 cad上增加一个新的"pysical"图层,表示物理上不可逾越的真实边界.  
-- 增加一个 xx_pysical_circle.json文件,保存真实边界的圆弧数据,数据格式为: id,圆弧中心x,圆弧中心y,半径,起始角度(弧度),终止角度(弧度)  
-- 增加一个 xx_pysical_line.json文件,保存真实边界的直线数据,数据格式为: id,起点x,起点y,终点x,终点y 

#### 2019.10.24 修改appendix的相关文件数据格式  
-- ref_line_appendix.json删除，新增appendix_belongs.json  
-- appendix_attribute.json : appendix_id, ref_line_id_total_number, ref_line_id_list, appendix_type, points_total_number, point0.x, point0.y, ...   
-- 每条参考线上的附属物id列表: appendix_belongs.json : ref_line_id, appendix_total_number, appendix_id_i, appendix_id_j, ... 
-- 增加physical_points文件 -- physical_points.json: physical_line_id: point_x, point_y