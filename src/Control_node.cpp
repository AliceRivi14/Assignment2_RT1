#include "ros/ros.h"
#include "sensor_msgs/LaserScan.h"
#include "second_assignment/Velocity.h"
#include "geometry_msgs/Twist.h"
#include "std_srvs/Empty.h"

ros::Publisher pubv; // global publisher

float acceleration = 0.0;
std_srvs::Empty reset;

// Function to calculate the minimum distance from an obstacle in a range of 720 elements
float RobotDistance(int min, int max, float dist_obs[])
{
  float dist_value = 25.0; // general distance
  for(int i = min; i <= max; i++)
  {
    if(dist_obs[i] <= dist_value)
      dist_value = dist_obs[i]; // minimum value
  }
  return dist_value;
}

// Function to handle the request input coming from ui_node
bool VelocityCallback(second_assignment::Velocity::Request &req, second_assignment::Velocity::Response &res)
{
  switch (req.input){
     // Input to increment the velocity
    case ('a'):
          acceleration += 0.5;
          req.input = 'x';
    break;
     // Input to decrement the velocity
    case ('d'):
          acceleration -= 0.5;
          req.input = 'x';
    break;
        // Input to reset the posisition
    case ('r'):
          ros::service::call("/reset_position",reset);
          req.input = 'x';
    break;
        // The velocity doesn't increment pressing another key
    case ('x'):
          return false;
    break;
    default:
          std::cout << "WRONG COMMAND! \n";
          fflush(stdout);
    break;
  }

  res.acc = acceleration;
  return true;
}
// Function to hande the datas about /base_scan topic
void LaserCallback(const sensor_msgs::LaserScan::ConstPtr& scan)
{
  float laser[721];
  for(int i=0; i<722; i++){
    laser[i] = scan->ranges[i];
  }

  // minimum distance from the wall on robot right,front and left
  float minR = RobotDistance(0, 100, laser);
  float minF = RobotDistance(300, 400, laser);
  float minL = RobotDistance(620, 720, laser);
  ROS_INFO("Distance: %f, %f, %f", minL, minF, minL);

  // control algorithm
  geometry_msgs::Twist vel;
  // obstacles in front of the robot
  if(minF < 1.5) {
      // robot is closer to the obstacles to the left
    if(minR < minL){
        vel.linear.x = 0.2;
        vel.angular.z = 1.0; // turn right
    }
      // robot is closer to the obstacles to the right
    else if (minL < minR){
        vel.linear.x = 0.2;
        vel.angular.z = -1.0; // turn left
    }
  }
  // no obstacles
  else {
      vel.linear.x = 1.5 + acceleration; //go straight and increment/decrement the velocity
      vel.angular.z = 0.0;
    }


  pubv.publish(vel); // publish on /cmd_vel topic
}

int main (int argc, char **argv)
{
  // initializing control_node
  ros::init(argc, argv, "control_node");
  ros::NodeHandle nh;

  ros::ServiceServer service = nh.advertiseService("/velocity", VelocityCallback);

  pubv = nh.advertise<geometry_msgs::Twist>("/cmd_vel",1);
  ros::Subscriber sub = nh.subscribe("/base_scan", 1, LaserCallback);

  ros::spin();
  return 0;
}
