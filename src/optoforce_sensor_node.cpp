#include <iostream>
#include "optoforce_sensor/opto.h"
#include "ros/ros.h"
#include "geometry_msgs/WrenchStamped.h"
#include <unistd.h>
#include <string.h>

void msSleep(unsigned long p_uMillisecs)
{
	usleep(p_uMillisecs * 1000);
}


int main(int argc, char **argv)
{
        
    OptoDAQ daq;
    OptoPorts ports;
    msSleep(2500); // We wait some ms to be sure about OptoPorts enumerated PortList

    double fx_gain, fy_gain, fz_gain;  // sensitivity gain in counts/N
    double tx_gain, ty_gain, tz_gain;  // sensitivity gain in count/Nm

    u_int8_t sens_params_set = 1;

    OPort* portlist=ports.listPorts(true);


    if (ports.getLastSize()>0)
    {
    	daq.open(portlist[0]);

        if (daq.getVersion()!=_95 && daq.getVersion() != _64) // It is a 3D sensor
        {
            OptoPackage pack3D;
            int size=daq.read(pack3D,false);	// Reading Sensor #0 (up to 16 Sensors)
            std::cout<<"x: "<<pack3D.x<<" y: "<<pack3D.y<<" z: "<<pack3D.z<<std::endl;
            std::cout<<"Sensor not supported in ROS yet"<<std::endl;
            //Support should be added later

        }
        else					  // It is a 6D sensor = the only one supported for now
        {
            OptoPackage6D pack6D;
            int size=daq.read6D(pack6D,false);
            //init ROS node
            std::cout<<"Found 6DOF sensor -> Starting ROS node"<<std::endl;
            ros::init(argc, argv, "OForceSensorPublisher");
            ros::NodeHandle n("~");
            //Creating wrench publisher
            ros::Publisher wrench_pub_raw = n.advertise<geometry_msgs::WrenchStamped>("OptoForceWrench_raw", 1000);
            ros::Publisher wrench_pub = n.advertise<geometry_msgs::WrenchStamped>("OptoForceWrench", 1000);

            // get sensitivy gains from rosparam server
            n.param("fx_gain", fx_gain, 0.0);
            n.param("fy_gain", fy_gain, 0.0);
            n.param("fz_gain", fz_gain, 0.0);

            n.param("tx_gain", tx_gain, 0.0);
            n.param("ty_gain", ty_gain, 0.0);
            n.param("tz_gain", tz_gain, 0.0);

            double double_thresh = 0.0001;

            if (fx_gain < 0.0001 || fy_gain < 0.0001 || fz_gain < 0.001) {
                std::cout<<"Sensitivity force params not set properly. Publishing only raw values."<<std::endl;
                sens_params_set = 0;
            }

            if (tx_gain < 0.0001 || ty_gain < 0.0001 || tz_gain < 0.001) {
                std::cout<<"Sensitivity torque params not set properly.Publishing only raw values."<<std::endl;
                sens_params_set = 0;
            }

            //Set ROS rate to 100 Hz
            ros::Rate loop_rate(100);
            //Main ROS loop
            while(ros::ok())
            {
                geometry_msgs::WrenchStamped wrench_msg; // Create msg
                //Fill msg
                wrench_msg.header.stamp = ros::Time::now();
                wrench_msg.wrench.force.x = pack6D.Fx;
                wrench_msg.wrench.force.y = pack6D.Fy;
                wrench_msg.wrench.force.z = pack6D.Fz;
                wrench_msg.wrench.torque.x = pack6D.Tx;
                wrench_msg.wrench.torque.y = pack6D.Ty;
                wrench_msg.wrench.torque.z = pack6D.Tz;
                wrench_pub_raw.publish(wrench_msg);

                if (sens_params_set) {
                    wrench_msg.header.stamp = ros::Time::now();
                    wrench_msg.wrench.force.x = pack6D.Fx / fx_gain;
                    wrench_msg.wrench.force.y = pack6D.Fy / fy_gain;
                    wrench_msg.wrench.force.z = pack6D.Fz / fz_gain;
                    wrench_msg.wrench.torque.x = pack6D.Tx / tx_gain;
                    wrench_msg.wrench.torque.y = pack6D.Ty / ty_gain;
                    wrench_msg.wrench.torque.z = pack6D.Tz / tz_gain;
                    wrench_pub.publish(wrench_msg);
                }

                ros::spinOnce();
                loop_rate.sleep();
                size = daq.read6D(pack6D,false);
                std::cout<<size<<std::endl;
            }
            //std::cout<<"Fx: "<<pack6D.Fx<<" Fy: "<<pack6D.Fy<<" Fz: "<<pack6D.Fz<<" ";
            //std::cout<<"Tx: "<<pack6D.Tx<<" Ty: "<<pack6D.Ty<<" Tz: "<<pack6D.Tz<<std::endl;
        }
        std::cout<<"Closing sensor connection"<<std::endl;
        daq.close();
    }
    else
    {
    	std::cout<<"No sensor available"<<std::endl;
    }
    return 0;
}
