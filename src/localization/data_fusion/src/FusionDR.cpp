#include "FusionDR.h"
#include "FusionCenter.h"
#include "CoordinateSystem.h"
#include <vector>
#include <eigen3/Eigen/Sparse>
#include "GlobalVari.h"



FusionDR::FusionDR(FusionCenter *pFsCenter)
{
    g_pfscenter = pFsCenter;
    memset(&g_last_tm,0,sizeof(UTC));
    g_fusion_X.resize(6);
    g_fusion_X = VectorXd::Zero(6,1);
    g_fusion_lX.resize(6);
    g_fusion_lX = VectorXd::Zero(6,1);
    g_fusion_lI.resize(6,6);
    g_fusion_lI = MatrixXd ::Zero(6,6);
}

FusionDR::~FusionDR(void)
{
    
}


// **************
// 功能:初始化状态参量
// 输入:无
// 输出:无
// 无返回
// ***************
void FusionDR::initForPQ(void)
{
    int count = 6;
    //估计均方误差P0,需改
    double P[count] = {0.01, 0.01, 0.01, 0.01, 0.01, 0.01};
    double Q[count] = {0.01, 0.01, 0.01, 0.01, 0.01, 0.01};

    g_fusion_P0.resize(count,count);
    g_fusion_P0 = MatrixXd::Zero(count,count);
    g_fusion_Q0.resize(count,count);
    g_fusion_Q0 = MatrixXd::Zero(count,count);
    for(int i = 0; i < count; i++)
    {
        for(int j = 0; j < count; j++)
        {
            if (i==j)
            {
                g_fusion_P0(i,j) = P[i]*P[j];
                g_fusion_Q0(i,j) = Q[i]*Q[j];
            }
            else
            {
                g_fusion_P0(i,j) = 0.0;
                g_fusion_Q0(i,j) = 0.0;
            }
        }
    }
}

// **************
// 功能:初始化状态参量X
// 输入:无
// 输出:无
// 无返回
// ***************
void FusionDR::initForX(void)
{
    g_fusion_lX(1,0) = 0.01;      //delta_ve
    g_fusion_lX(2,0) = 0.01;      //delta_vn
    g_fusion_lX(3,0) = 0.01;      //delta_vu
    g_fusion_lX(4,0) = 0.01;      //delta_nav_lan
    g_fusion_lX(5,0) = 0.01;      //delta_nav_lon
    g_fusion_lX(6,0) = 0.01;      //delta_nav_h
}

// **************
// 功能:初始化状态参量的测量方差R
// 输入:无
// 输出:无
// 无返回
// ***************
void FusionDR::initForR(void)
{
    int count = 4;
    double R[count] = {0.01, 0.01, 0.01, 0.01};
    g_fusion_R.resize(count,count);
    g_fusion_R = MatrixXd::Zero(count,count);
    for(int i = 0; i < count; i++)
    {
        for(int j = 0; j < count; j++)
        {
            if (i==j) 
            {
                g_fusion_R(i,j) = R[i]*R[j];
            }
            else
            {
                g_fusion_R(i,j) = 0.0;
            }
        }
    }
}

// **************
// 功能:计算导航坐标系下(东北天)指令角速度wn_in及两个曲率半径
// 输入:LLH[3] 纬经高,VENU[3]东北天速度
// 输出:wn_in 导航坐标系下指令角速度  R_NM 两个曲率半径,R_NM[0] 沿卯酉圈的曲率半径 R_NM[1] 沿子午圈的曲率半径
// 无返回
// ***************
void FusionDR::initForNavigationParam(double (&LLH)[3], double (&VENU)[3], Vector3d &wn_in, double (&R_NM)[2])
{
    double LLH1[3];
    for(int i = 0; i < 2; i++)
    {
        LLH1[i] = LLH[i]*M_PI/180;
    }
    R_NM[0] = Ra*(1 - 2*ff + 3*ff*pow(sin(LLH1[0]),2));
    R_NM[1] = Ra*(1 + ff*pow(sin(LLH1[0]),2));
    Vector3d wn_ie;     //导航坐标系下跟着地球转的角速度
    wn_ie(0,0) = 0.0;
    wn_ie(1,0) = w_ie*cos(LLH1[0]);
    wn_ie(2,0) = w_ie*sin(LLH1[0]);

    Vector3d wn_en;        //由于运载体运动而引起的相对地球的旋转角速度
    wn_en(0,0) = -VENU[1]/R_NM[1];
    wn_en(1,0) = VENU[0]/R_NM[0];
    wn_en(2,0) = VENU[0]/R_NM[0]*tan(LLH1[0]);

    wn_in = wn_ie + wn_en; 
}


void FusionDR::calculateFtSysParam(PoseResult &h_fm, PoseResult &l_fm,
                            MatrixXd &fusion_F,MatrixXd &fusion_G)
{
    double LLH[3],VENU[3],R_NM[2];
    Vector3d wn_in;
    wn_in = Vector3d::Zero(3,1);
    LLH[0] = h_fm.pos.lan;
    LLH[1] = h_fm.pos.lon;
    LLH[2] = h_fm.pos.h;
    VENU[0] = h_fm.vel.venu.vx;
    VENU[1] = h_fm.vel.venu.vy;
    VENU[2] = h_fm.vel.venu.vz;
    initForNavigationParam(LLH, VENU, wn_in, R_NM);
    int sys_count = 6;
    fusion_F.resize(sys_count,sys_count);
    fusion_F.setZero(sys_count,sys_count);

    fusion_F(0,0) = (h_fm.vel.venu.vy*tan(h_fm.pos.lan*M_PI/180)
                        -
                        h_fm.vel.venu.vz)/(R_NM[0] + h_fm.pos.h);
    fusion_F(0,1) = 2*w_ie*sin(h_fm.pos.lan*M_PI/180)
                        +
                        h_fm.vel.venu.vx/(R_NM[0] + h_fm.pos.h)*tan(h_fm.pos.lan*M_PI/180);
    fusion_F(0,2) = -(2*w_ie*cos(h_fm.pos.lan*M_PI/180) + h_fm.vel.venu.vx/(R_NM[0] + h_fm.pos.h));
    fusion_F(0,3) = 2*w_ie*(h_fm.vel.venu.vz*sin(h_fm.pos.lan*M_PI/180) 
                        +
                        h_fm.vel.venu.vy*cos(h_fm.pos.lan*M_PI/180))
                        + 
                        h_fm.vel.venu.vx*h_fm.vel.venu.vy/(R_NM[0] + h_fm.pos.h)
                        /
                        pow(cos(h_fm.pos.lan*M_PI/180),2); 
    fusion_F(0,5) = (h_fm.vel.venu.vx*h_fm.vel.venu.vz 
                        - 
                        h_fm.vel.venu.vx*h_fm.vel.venu.vy*tan(h_fm.pos.lan*M_PI/180))
                        /
                        pow(R_NM[0] + h_fm.pos.h,2);

    fusion_F(1,0) = -2*(w_ie*sin(h_fm.pos.lan*M_PI/180) 
                        + 
                        h_fm.vel.venu.vx/(R_NM[0] + h_fm.pos.h)
                        * 
                        tan(h_fm.pos.lan*M_PI/180));
    fusion_F(1,1) = -h_fm.vel.venu.vz/(R_NM[1] + h_fm.pos.h);
    fusion_F(1,2) = -h_fm.vel.venu.vy/(R_NM[1] + h_fm.pos.h);
    fusion_F(1,3) = -(2*h_fm.vel.venu.vx*w_ie*cos(h_fm.pos.lan) 
                        + 
                        pow(h_fm.vel.venu.vx,2)/(R_NM[0] + h_fm.pos.h)
                        /
                        pow(cos(h_fm.pos.lan*M_PI/180),2));
    fusion_F(1,5) = h_fm.vel.venu.vy*h_fm.vel.venu.vz
                        /
                        pow(R_NM[1]+h_fm.pos.h,2) 
                        + 
                        pow(h_fm.vel.venu.vx,2)*tan(h_fm.pos.lan*M_PI/180)
                        /
                        pow(R_NM[0] + h_fm.pos.h,2);

    fusion_F(2,0) = 2*(w_ie*cos(h_fm.pos.lan*M_PI/180) 
                        + 
                        h_fm.vel.venu.vx
                        /
                        (R_NM[0] + h_fm.pos.h));
    fusion_F(2,1) = 2*h_fm.vel.venu.vy/(R_NM[1] + h_fm.pos.h);
    fusion_F(2,3) = -2*h_fm.vel.venu.vx*w_ie*sin(h_fm.pos.lan*M_PI/180);
    fusion_F(2,5) = -(pow(h_fm.vel.venu.vy,2)/pow(R_NM[1] + h_fm.pos.h,2) 
                        + 
                        pow(h_fm.vel.venu.vx,2)/pow(R_NM[0] + h_fm.pos.h,2));

    fusion_F(3,1) = 1/(R_NM[1] + h_fm.pos.h);
    fusion_F(3,5) = -h_fm.vel.venu.vy/pow(R_NM[1] + h_fm.pos.h,2);

    fusion_F(4,0) = 1/cos(h_fm.pos.lan*M_PI/180)/(R_NM[0] + h_fm.pos.h);
    fusion_F(4,3) = h_fm.vel.venu.vx/(R_NM[0] + h_fm.pos.h)*tan(h_fm.pos.lan*M_PI/180)/cos(h_fm.pos.lan*M_PI/180);
    fusion_F(4,5) = -h_fm.vel.venu.vx/cos(h_fm.pos.lan*M_PI/180)/pow(R_NM[0] + h_fm.pos.h,2);

    fusion_F(5,2) = 1;

    //fusion_G:
    fusion_G.resize(sys_count,sys_count);
    fusion_G.setZero(sys_count,sys_count);      //系统噪声矩阵
    fusion_G.setIdentity(sys_count,sys_count);   //根据系统误差选定fusion_G
}

// **************
// 功能:计算测量方程H矩阵
// 输入:h_fm 组合导航数据
// 输出:fusion_H H矩阵,根据低频传感器的类型H矩阵不同
// 无返回
// ***************
void FusionDR::calculateFtMeaParam(PoseResult &h_fm,MatrixXd &fusion_H)
{
    fusion_H.resize(4,6);
    fusion_H.setZero(4,6);

    fusion_H(0,0) = 1;
    fusion_H(1,1) = 1;
    fusion_H(2,3) = 1;
    fusion_H(3,4) = 1;   
}

// **************
// 功能:一步转移矩阵和等效离散系统噪声方差阵的计算
// 输入:g_fusion_F时间更新的传递矩阵 g_fusion_G系统噪声矩阵,时间配准时刻time
// 输出:离散后的一步转移矩阵g_fusion_Fai,g_fusion_Q离散化后的系统误差方差阵
// 无返回
// ***************
void FusionDR::discreteForFusionFG(MatrixXd &fusion_F, MatrixXd &fusion_G, MatrixXd &fusion_Q0, double &delta_T,
                            MatrixXd &g_fusion_Fai, MatrixXd &g_fusion_Q)
{
    MatrixXd Mat_I;
    MatrixXd Mat_tp1;
    MatrixXd Mat_tp2;
    int sys_count = 6;

    //一步转移矩阵
    Mat_I.resize(sys_count,sys_count);
    Mat_I.setIdentity(sys_count,sys_count); 
    Mat_tp1.resize(sys_count,sys_count);
    Mat_tp1 = delta_T*fusion_F;
    Mat_tp2.resize(sys_count,sys_count);
    Mat_tp2 = pow(delta_T,2)/2*fusion_F*fusion_F;
    g_fusion_Fai.resize(sys_count,sys_count);
    g_fusion_Fai.Zero(sys_count,sys_count);
    g_fusion_Fai = Mat_I + Mat_tp1 + Mat_tp2;

    //系统噪声离散化
    MatrixXd M_tp1;
    M_tp1.resize(sys_count,sys_count);
    M_tp1.setZero(sys_count,sys_count);
    MatrixXd M_tp2;
    M_tp2.resize(sys_count,sys_count);
    M_tp2.setZero(sys_count,sys_count);
    M_tp1 = fusion_G*fusion_Q0*fusion_G.transpose();
    MatrixXd temp;
    temp.resize(sys_count,sys_count);
    temp.setZero(sys_count,sys_count);
    temp = fusion_F*M_tp1;
    M_tp2 = temp + temp.transpose();
    g_fusion_Q = M_tp1*delta_T + M_tp2*pow(delta_T,2)/2;
}

// **************
// 功能:计算测量方程的测量参数Z
// 输入:high_freq_match组合导航,low_freq_match低频传感器,DR传进的位置信息是港区坐标系的位置(单位m)
// 输出:fusion_Z测量参数Z,在此更改了low_fre_match的pos值,为DR计算的经纬高
// 无返回
// ***************
void FusionDR::calculateMeaZ(PoseResult &high_freq_match, PoseResult &low_freq_match, VectorXd &fusion_Z)
{
    //计算lon0
    double LLH_TP[3] = {high_freq_match.pos.lan,high_freq_match.pos.lon,high_freq_match.pos.h};
    double lon0;
    double UTM_TP[3];
    lon0 = transForLLHtoUTM(LLH_TP,UTM_TP);
    //计算DR推算的LLH;
    double utm[3] = {0,0,0};
    utm[0] = low_freq_match.pos.lan + Har_e0;
    utm[1] = low_freq_match.pos.lon + Har_n0;
    utm[2] = high_freq_match.pos.h;
    double DR_LLH[3] = {0,0,0};
    transForUTMtoLLH(utm,DR_LLH,lon0);
    low_freq_match.pos.lan = DR_LLH[0];
    low_freq_match.pos.lon = DR_LLH[1];
    low_freq_match.pos.h = DR_LLH[2];
    //计算两个传感器位置差
    double lan_lon_deg[2];
    lan_lon_deg[0] = high_freq_match.pos.lan - low_freq_match.pos.lan;
    lan_lon_deg[1] = high_freq_match.pos.lon - low_freq_match.pos.lon;
    double lan_lon_m[2] = {0,0};
    transForDegreetoMeter(lan_lon_deg,DR_LLH,lan_lon_m);
    //计算测量参数
    fusion_Z.resize(4,1);
    fusion_Z(0) = high_freq_match.vel.venu.vx - low_freq_match.vel.venu.vx;
    fusion_Z(1) = high_freq_match.vel.venu.vy - low_freq_match.vel.venu.vy;
    fusion_Z(2) = lan_lon_m[0];
    fusion_Z(3) = lan_lon_m[1];

}


void FusionDR::calculatePoseConfidence(VectorXd &fusion_Z, MatrixXd &fusion_H, MatrixXd &I_k, float &pose_confidence)
{
    int count_lan = 2,count_lon = 3;
    int countr = 4;
    MatrixXd temp1;
    MatrixXd temp2;
    temp1.resize(countr,6);
    temp1.setZero(countr,6);
    temp1 = fusion_H*g_fusion_X;
    temp2.resize(countr,countr);
    temp2.setZero(countr,countr);
    Vector2d temp_z;
    Vector2d temp_z2;
    temp_z(0,0) = fusion_Z(count_lan);
    temp_z(1,0) = fusion_Z(count_lon);
    temp_z2(0,0) = temp1(count_lan);
    temp_z2(1,0) = temp1(count_lon);
    Vector2d delta_z;
    delta_z = Vector2d::Zero();
    delta_z = temp_z - temp_z2;
    double meas = 1.0;
    meas = delta_z.transpose()*delta_z;

    MatrixXd tpP_k;
    tpP_k.resize(6,6);
    tpP_k.setZero(6,6);
    tpP_k = I_k.inverse();

    temp2 = fusion_H*tpP_k*fusion_H.transpose() + g_fusion_R;
    double theory = 1.0;
    theory = temp2(count_lan,count_lan) + temp2(count_lon,count_lon);
    if (meas <= theory)
        {
            pose_confidence = meas/theory;
        }
    else
    {
        pose_confidence = theory/meas;
    } 
}

// **************
// 功能:滤波计算
// 输入:high_freq_match 本帧高频传感器结果 low_freq_match 本帧低频传感器结果,time融合时刻
// 输出:融合结果g_fusion_data
// 返回:true
// ***************
bool FusionDR::calculateFilter(PoseResult &high_freq_match, PoseResult &low_freq_match,UTC &time,
                        location_msgs::FusionDataInfo &g_fusion_data)
{
    //状态参数初始化
    if (g_pfscenter->g_lkinit_flag == false ) 
    {
        initForX();
        initForPQ();
        g_fusion_lI = g_fusion_P0.inverse();  //信息矩阵
    }
    initForR();
    MatrixXd fusion_F;      //时间更新的传递矩阵
    MatrixXd   fusion_H;                //量测更新的矩阵
    MatrixXd  fusion_G;      //系统噪声矩阵
    int count_f = 6;
    calculateFtSysParam(high_freq_match, low_freq_match,fusion_F,fusion_G);
    calculateFtMeaParam(high_freq_match,fusion_H);
    double delta_T;
    delta_T = time.hour*3600 + time.min*60 + time.sec + time.msec/1000
              - (g_last_tm.hour*3600 + g_last_tm.min*60 + g_last_tm.sec + g_last_tm.msec/1000);
    if (delta_T > 1) 
    {
        delta_T = 0.0333;
    }
    MatrixXd fusion_Fai;      //离散化后的时间更新的传递矩阵
    MatrixXd fusion_Q;     //离散化后的系统误差方差阵
    discreteForFusionFG(fusion_F, fusion_G, g_fusion_Q0, delta_T,fusion_Fai,fusion_Q);

    MatrixXd X_2k;       //一步预测状态参量(从k-1至k)
    X_2k.resize(count_f,1);
    X_2k.setZero(count_f,1);
    X_2k = fusion_Fai*g_fusion_lX;

    MatrixXd R_inv;
    //滤波增益
    MatrixXd K_k;       //当前时刻的滤波增益
    int count_R = 4;
    R_inv.resize(count_R,count_R);
    R_inv = MatrixXd::Zero(count_R,count_R);
    K_k.resize(count_f,count_R);
    K_k.setZero(count_f,count_R);

    //用于稀疏矩阵求逆
    typedef SparseMatrix<double,ColMajor,int> SpMatType;
    typedef Triplet<double> Tpt;
    //求fusion_Q的逆矩阵
    int Q_nonzero_num = fusion_Q.nonZeros();
    int Q_nonzero_r[Q_nonzero_num];
    int Q_nonzero_c[Q_nonzero_num];
    int Qcount_size = 0;
    int Qrows = fusion_Q.rows(),Qcols = fusion_Q.cols();
    for (int j = 0; j < Qcols; j++)
    {
        for (int i = 0; i < Qrows; i++)
        {
            if (fusion_Q(i,j) != 1e-8)
            {
                Q_nonzero_r[Qcount_size] = i;
                Q_nonzero_c[Qcount_size] = j;
                Qcount_size++;
            }   
        } 
    }
    vector<Tpt> Q_tripletlist;
    Q_tripletlist.reserve(Q_nonzero_num);   
    for (int i = 0; i < Q_nonzero_num; i++)
    {
        double val = fusion_Q(Q_nonzero_r[i],Q_nonzero_c[i]);
        Q_tripletlist.push_back(Tpt(Q_nonzero_r[i],Q_nonzero_c[i],val));
    }
    SpMatType Q_spA(Qrows,Qcols);
    Q_spA.setFromTriplets(Q_tripletlist.begin(),Q_tripletlist.end());
    Q_spA.makeCompressed(); //压缩优化矩阵
    vector<Tpt>().swap(Q_tripletlist);
    SparseQR<SpMatType,COLAMDOrdering<int> > Q_solver;
    Q_solver.compute(Q_spA);
    SpMatType Q_Sp_I(Qrows,Qcols);
    Q_Sp_I.setIdentity();
    MatrixXd Q_inv(Qrows,Qcols);
    Q_inv = Q_solver.solve(Q_Sp_I);
    R_inv = g_fusion_R.inverse();

    //计算信息矩阵
    MatrixXd I_2k;      //信息矩阵(从k-1至k)
    I_2k.resize(count_f,count_f);
    I_2k.setZero(count_f,count_f);
    MatrixXd I_k;       //k时刻信息矩阵
    I_k.resize(count_f,count_f);
    I_k.setZero(count_f,count_f);
    MatrixXd temp_1;
    temp_1.resize(count_f,count_f);
    temp_1 = MatrixXd::Zero(count_f,count_f);
    temp_1 = fusion_Fai.transpose()*Q_inv*fusion_Fai;
    MatrixXd temp_2;
    temp_2.resize(count_f,count_f);
    temp_2 = MatrixXd::Zero(count_f,count_f);
    temp_2 = g_fusion_lI + temp_1;
    MatrixXd temp_3;
    temp_3.resize(count_f,count_f);
    temp_3 = MatrixXd::Zero(count_f,count_f);
    temp_3 = Q_inv*fusion_Fai*temp_2.inverse();
    MatrixXd temp_4;
    temp_4.resize(count_f,count_f);
    temp_4 = MatrixXd::Zero(count_f,count_f);
    temp_4 = temp_3*fusion_Fai.transpose()*Q_inv;
    I_2k = Q_inv - temp_4;
    //信息矩阵
    I_k = I_2k + fusion_H.transpose()*R_inv*fusion_H;
    //求稀疏矩阵I_k的逆矩阵
    int nonzero_num = I_k.nonZeros();   //非零元素个数
    int nonzero_r[nonzero_num];
    int nonzero_c[nonzero_num];
    int count_size = 0;
    int rows = I_k.rows(),cols = I_k.cols();
    for (int j = 0; j < cols; j++)
    {
        for (int i = 0; i < rows; i++)
        {
            if (I_k(i,j) != 1e-8)
            {
                nonzero_r[count_size] = i;
                nonzero_c[count_size] = j;
                count_size++;
            }   
        } 
    }
    vector<Tpt> tripletlist;
    tripletlist.reserve(nonzero_num);   //为25个非零元素分配空间
    for (int i = 0; i < nonzero_num; i++)
    {
        double val = I_k(nonzero_r[i],nonzero_c[i]);
        tripletlist.push_back(Tpt(nonzero_r[i],nonzero_c[i],val));
    }
    SpMatType spA(rows,cols);
    spA.setFromTriplets(tripletlist.begin(),tripletlist.end());
    spA.makeCompressed(); //压缩优化矩阵
    vector<Tpt>().swap(tripletlist);
    SparseQR<SpMatType,COLAMDOrdering<int> > solver;
    solver.compute(spA);
    SpMatType Sp_I(rows,cols);
    Sp_I.setIdentity();
    SpMatType I_k_inv(rows,cols);
    I_k_inv = solver.solve(Sp_I);

    //滤波增益
    K_k = I_k.inverse()*fusion_H.transpose()*R_inv;

    //当前时刻状态参数
    VectorXd fusion_Z;      //  测量量
    calculateMeaZ(high_freq_match,low_freq_match,fusion_Z);

    g_fusion_X = X_2k + K_k*(fusion_Z - fusion_H*X_2k);

    //计算置信度
    float pose_confidence = 0.0;
    calculatePoseConfidence(fusion_Z, fusion_H, I_k,pose_confidence);
    g_fusion_data.pose_confidence = pose_confidence;

    //融合结果
    //姿态角
    g_fusion_data.yaw = high_freq_match.att.yaw;
    g_fusion_data.pitch = high_freq_match.att.pitch;
    g_fusion_data.roll = high_freq_match.att.roll;

    //计算补偿后的速度
    g_fusion_data.velocity.linear.x = high_freq_match.vel.venu.vx + g_fusion_X(0);
    g_fusion_data.velocity.linear.y = high_freq_match.vel.venu.vy + g_fusion_X(1);
    g_fusion_data.velocity.linear.z = high_freq_match.vel.venu.vz + g_fusion_X(2);

    //计算补偿后的位置
    double lan_lon_m[2] = {g_fusion_X(2),g_fusion_X(3)};
    double lan_lon_deg[2];
    double LLH_new[3] = {high_freq_match.pos.lan,high_freq_match.pos.lon,high_freq_match.pos.h};
    transForMetertoDegree(lan_lon_m,LLH_new,lan_lon_deg);
    g_fusion_data.pose_llh.x = high_freq_match.pos.lan + lan_lon_deg[0];
    g_fusion_data.pose_llh.y = high_freq_match.pos.lon + lan_lon_deg[1];
    g_fusion_data.pose_llh.z = high_freq_match.pos.h + g_fusion_X(5);

    //加速度
    g_fusion_data.accel.linear.x = high_freq_match.accel.ax;
    g_fusion_data.accel.linear.y = high_freq_match.accel.ay;
    g_fusion_data.accel.linear.z = high_freq_match.accel.az;

    //车体坐标系下角速度
    g_fusion_data.velocity.angular.x = high_freq_match.vel.wxyz.wx;
    g_fusion_data.velocity.angular.y = high_freq_match.vel.wxyz.wy;
    g_fusion_data.velocity.angular.z = high_freq_match.vel.wxyz.wz;

    //传感器融合状态
    g_fusion_data.fuse_state = 14;
    g_pfscenter->g_fusion_data_calib = g_fusion_data;

    //存储结果用于下次滤波
    g_last_tm = time;
    //--------test
    g_last_tm.hour = 2;
    g_last_tm.min = 30;
    g_last_tm.sec = 30;
    g_last_tm.msec = 98;
    g_fusion_lI = I_k;
    g_fusion_lX = g_fusion_X;

    return true;
}