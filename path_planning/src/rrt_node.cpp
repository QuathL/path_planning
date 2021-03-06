#include <ros/ros.h>
#include <visualization_msgs/Marker.h>
#include <nav_msgs/Path.h>
#include <geometry_msgs/Point.h>
#include <path_planning/rrt.h>
#include <path_planning/obstacles.h>
#include <iostream>
#include <cmath>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <time.h>
#include <std_msgs/String.h>
#include <cstdlib>
#include <string>
#include <fstream>

using namespace std;

#define success false
#define running true

using namespace rrt;

bool status = running;
vector< vector<geometry_msgs::Point> >  obstacleList;
visualization_msgs::Marker start2endPoint_msg;
visualization_msgs::Marker currentPath;
bool alreadySetPath = false;

void initializeMarkers(visualization_msgs::Marker &sourcePoint,
    visualization_msgs::Marker &goalPoint,
    visualization_msgs::Marker &randomPoint,
    visualization_msgs::Marker &rrtTreeMarker,
    visualization_msgs::Marker &finalPath)
{
    //init headers
	sourcePoint.header.frame_id    = goalPoint.header.frame_id    = randomPoint.header.frame_id    = rrtTreeMarker.header.frame_id    = finalPath.header.frame_id    = "path_planner";
	sourcePoint.header.stamp       = goalPoint.header.stamp       = randomPoint.header.stamp       = rrtTreeMarker.header.stamp       = finalPath.header.stamp       = ros::Time::now();
	sourcePoint.ns                 = goalPoint.ns                 = randomPoint.ns                 = rrtTreeMarker.ns                 = finalPath.ns                 = "path_planner";
	sourcePoint.action             = goalPoint.action             = randomPoint.action             = rrtTreeMarker.action             = finalPath.action             = visualization_msgs::Marker::ADD;
	sourcePoint.pose.orientation.w = goalPoint.pose.orientation.w = randomPoint.pose.orientation.w = rrtTreeMarker.pose.orientation.w = finalPath.pose.orientation.w = 1.0;

    //setting id for each marker
  sourcePoint.id    = 0;
	goalPoint.id      = 1;
	randomPoint.id    = 2;
	rrtTreeMarker.id  = 3;
  finalPath.id      = 4;

	//defining types
	rrtTreeMarker.type                                    = visualization_msgs::Marker::LINE_LIST;
	finalPath.type                                        = visualization_msgs::Marker::LINE_STRIP;
	sourcePoint.type  = goalPoint.type = randomPoint.type = visualization_msgs::Marker::SPHERE;

	//setting scale
	rrtTreeMarker.scale.x = 0.2;
	finalPath.scale.x     = 1;
	sourcePoint.scale.x   = goalPoint.scale.x = randomPoint.scale.x = 2;
  sourcePoint.scale.y   = goalPoint.scale.y = randomPoint.scale.y = 2;
  sourcePoint.scale.z   = goalPoint.scale.z = randomPoint.scale.z = 1;

    //assigning colors
	sourcePoint.color.g   = 1.0f;
	goalPoint.color.r     = 1.0f;
  randomPoint.color.b   = 1.0f;

	rrtTreeMarker.color.r = 0.8f;
	rrtTreeMarker.color.g = 0.4f;

	finalPath.color.r = 0.2f;
	finalPath.color.g = 0.2f;
	finalPath.color.b = 1.0f;

	sourcePoint.color.a = goalPoint.color.a = randomPoint.color.a = rrtTreeMarker.color.a = finalPath.color.a = 1.0f;
}

// vector< vector<geometry_msgs::Point> > getObstacles()
// {
//     geometry_msgs::Point start_point;
//     start_point.x = 20;
//     start_point.y = 20;
//     start_point.z = 0;
//
//     obstacles obst;
//     return obst.getObstacleArray(start_point);
// }

void addBranchtoRRTTree(visualization_msgs::Marker &rrtTreeMarker, RRT::rrtNode &tempNode, RRT &myRRT)
{

geometry_msgs::Point point;

point.x = tempNode.posX;
point.y = tempNode.posY;
point.z = 0;
rrtTreeMarker.points.push_back(point);

RRT::rrtNode parentNode = myRRT.getParent(tempNode.nodeID);

point.x = parentNode.posX;
point.y = parentNode.posY;
point.z = 0;

rrtTreeMarker.points.push_back(point);
}

bool checkIfInsideBoundary(RRT::rrtNode &tempNode)
{
    if(tempNode.posX < 0 || tempNode.posY < 0  || tempNode.posX > 100 || tempNode.posY > 100 ) return false;
    else return true;
}

bool checkIfOutsideObstacles(vector< vector<geometry_msgs::Point> > &obstArray, RRT::rrtNode &tempNode)
{
    double AB, AD, AMAB, AMAD;

    for(int i=0; i<obstArray.size(); i++)
    {
        AB = (pow(obstArray[i][0].x - obstArray[i][1].x,2) + pow(obstArray[i][0].y - obstArray[i][1].y,2));
        AD = (pow(obstArray[i][0].x - obstArray[i][3].x,2) + pow(obstArray[i][0].y - obstArray[i][3].y,2));
        AMAB = (((tempNode.posX - obstArray[i][0].x) * (obstArray[i][1].x - obstArray[i][0].x)) + (( tempNode.posY - obstArray[i][0].y) * (obstArray[i][1].y - obstArray[i][0].y)));
        AMAD = (((tempNode.posX - obstArray[i][0].x) * (obstArray[i][3].x - obstArray[i][0].x)) + (( tempNode.posY - obstArray[i][0].y) * (obstArray[i][3].y - obstArray[i][0].y)));
         //(0<AM⋅AB<AB⋅AB)∧(0<AM⋅AD<AD⋅AD)
        if((0 < AMAB) && (AMAB < AB) && (0 < AMAD) && (AMAD < AD))
        {
            return false;
        }
    }
    return true;
}

void generateTempPoint(RRT::rrtNode &tempNode)
{
    int x = rand() % 150 + 1;
    int y = rand() % 150 + 1;
    //std::cout<<"Random X: "<<x <<endl<<"Random Y: "<<y<<endl;
    tempNode.posX = x;
    tempNode.posY = y;
}

bool addNewPointtoRRT(RRT &myRRT, RRT::rrtNode &tempNode, int rrtStepSize, vector< vector<geometry_msgs::Point> > &obstArray)
{
    int nearestNodeID = myRRT.getNearestNodeID(tempNode.posX,tempNode.posY);

    RRT::rrtNode nearestNode = myRRT.getNode(nearestNodeID);

    double theta = atan2(tempNode.posY - nearestNode.posY,tempNode.posX - nearestNode.posX);

    tempNode.posX = nearestNode.posX + (rrtStepSize * cos(theta));
    tempNode.posY = nearestNode.posY + (rrtStepSize * sin(theta));

    if(checkIfInsideBoundary(tempNode) && checkIfOutsideObstacles(obstArray,tempNode))
    {
        tempNode.parentID = nearestNodeID;
        tempNode.nodeID = myRRT.getTreeSize();
        myRRT.addNewNode(tempNode);
        return true;
    }
    else
        return false;
}

bool checkNodetoGoal(int X, int Y, RRT::rrtNode &tempNode)
{
    double distance = sqrt(pow(X-tempNode.posX,2)+pow(Y-tempNode.posY,2));
    if(distance < 3)
    {
        return true;
    }
    return false;
}

void setFinalPathData(vector< vector<int> > &rrtPaths, RRT &myRRT, int i, visualization_msgs::Marker &finalpath, int goalX, int goalY)
{
    RRT::rrtNode tempNode;
    geometry_msgs::Point point;
    for(int j=0; j<rrtPaths[i].size();j++)
    {
        tempNode = myRRT.getNode(rrtPaths[i][j]);

        point.x = tempNode.posX;
        point.y = tempNode.posY;
        point.z = 0;

        finalpath.points.push_back(point);
    }

    point.x = goalX;
    point.y = goalY;
    finalpath.points.push_back(point);
}

void RRT_path_planning(const visualization_msgs::Marker msg)
{
  visualization_msgs::Marker sourcePoint;
  visualization_msgs::Marker goalPoint;
  visualization_msgs::Marker randomPoint;
  visualization_msgs::Marker rrtTreeMarker;
  visualization_msgs::Marker finalPath;

  status = running;
  initializeMarkers(sourcePoint, goalPoint, randomPoint, rrtTreeMarker, finalPath);

  ros::NodeHandle n;

  //defining Publisher
  ros::Publisher rrt_publisher = n.advertise<visualization_msgs::Marker>("path_planner_rrt",1);

  ROS_INFO("UAV_id : ");
  std::cout << msg.text << "\n";
  ROS_INFO("UAV_Start : ");
  std::cout << msg.points[0] << "\n";
  ROS_INFO("UAV_Goal : ");
  std::cout << msg.points[1] << "\n";

  //setting source and goal points
  sourcePoint.pose.position.x = msg.points[0].x;
  sourcePoint.pose.position.y = msg.points[0].y;

  goalPoint.pose.position.x = msg.points[1].x;
  goalPoint.pose.position.y = msg.points[1].y;

  rrt_publisher.publish(sourcePoint);
  rrt_publisher.publish(goalPoint);
  srand (time(NULL));
  //initialize rrt specific variables

  //initializing rrtTree
  RRT myRRT(sourcePoint.pose.position.x, sourcePoint.pose.position.y);
  int goalX, goalY;
  goalX =  goalPoint.pose.position.x;
  goalY =  goalPoint.pose.position.y;

  int rrtStepSize = 1;

  vector< vector<int> > rrtPaths;
  vector<int> path;
  int rrtPathLimit = 1;

  int shortestPathLength = 9999;
  int shortestPath = -1;

  RRT::rrtNode tempNode;

  // vector< vector<geometry_msgs::Point> >  obstacleList = getObstacles();

  bool addNodeResult = false, nodeToGoal = false;

  while(status){
    if(rrtPaths.size() < rrtPathLimit)
    {
      generateTempPoint(tempNode);
      //std::cout<<"tempnode generated"<<endl;
      addNodeResult = addNewPointtoRRT(myRRT,tempNode,rrtStepSize,obstacleList);
      if(addNodeResult)
      {
        // std::cout<<"tempnode accepted"<<endl;
        addBranchtoRRTTree(rrtTreeMarker,tempNode,myRRT);
        // std::cout<<"tempnode printed"<<endl;
        nodeToGoal = checkNodetoGoal(goalX, goalY,tempNode);
        if(nodeToGoal)
        {
          path = myRRT.getRootToEndPath(tempNode.nodeID);
          rrtPaths.push_back(path);
          std::cout<<"New Path Found. Total paths "<<rrtPaths.size()<<endl;
          //ros::Duration(10).sleep();
          //std::cout<<"got Root Path"<<endl;
        }
      }
    }
    else //if(rrtPaths.size() >= rrtPathLimit)
    {
      status = success;
      std::cout<<"Finding Optimal Path"<<endl;
      for(int i=0; i<rrtPaths.size();i++)
      {
        if(rrtPaths[i].size() < shortestPath)
        {
          shortestPath = i;
          shortestPathLength = rrtPaths[i].size();
        }
      }
      setFinalPathData(rrtPaths, myRRT, shortestPath, finalPath, goalX, goalY);
      finalPath.text = msg.text;
      rrt_publisher.publish(finalPath);
      currentPath = finalPath;
    }

    rrt_publisher.publish(sourcePoint);
    rrt_publisher.publish(goalPoint);
    // rrt_publisher.publish(rrtTreeMarker);
    // rrt_publisher.publish(finalPath);
    ros::spinOnce();
    ros::Duration(0.01).sleep();
  }

  std::cout << "FinalPath length " << finalPath.points.size() << "\n";
  // std::cout << "Final Path: \n" << finalPath<< "\n";
  // std::cout << "Finished Callback\n";
  //visualization_msgs::Marker finalPath = visualization_msgs::Marker();
  //resetMarkers(sourcePoint, goalPoint, randomPoint, rrtTreeMarker, finalPath);
}

void Callback_pp(const visualization_msgs::Marker msg)
{
  start2endPoint_msg = msg;
}

void Callback_obst(const visualization_msgs::Marker msg)
{
  geometry_msgs::Point start_point;
  start_point = msg.points[0];

  obstacles obst;
  obstacleList = obst.getObstacleArray(start_point);

  if (alreadySetPath)
  {
    RRT::rrtNode tempNode;
    // std::cout<<"Before the loop."<<endl;
    std::cout << "Current path length: " << currentPath.points.size() << endl;
    for(int i=0; i<currentPath.points.size(); i++)
    {
      // std::cout<<"Inside the loop."<<endl;
      tempNode.posX = currentPath.points[i].x;
      tempNode.posY = currentPath.points[i].y;
      if (!checkIfOutsideObstacles(obstacleList, tempNode))
      {
        std::cout<<"Path conflicts with obstacle! Generating a new path..."<<endl;
        RRT_path_planning(start2endPoint_msg);
        break;
      }
    }
    // std::cout<<"After the loop."<<endl;
  }
  else
  {
    RRT_path_planning(start2endPoint_msg);
    alreadySetPath = true;
  }

}

int main(int argc,char** argv)
{
    geometry_msgs::Point tempStart;
    geometry_msgs::Point tempEnd;
    tempStart.x = 1;
    tempStart.y = 1;
    tempStart.z = 1;
    tempEnd.x = 10;
    tempEnd.y = 10;
    tempEnd.z = 1;
    start2endPoint_msg.points.push_back(tempStart);
    start2endPoint_msg.points.push_back(tempEnd);

    //initializing ROS
    ros::init(argc,argv,"rrt_node");
    ros::NodeHandle n;

    //declaring Publisher
    ros::Publisher rrt_publisher = n.advertise<visualization_msgs::Marker>("path_planner_rrt",1);

    //initializePath(finalPath);
    visualization_msgs::Marker sourcePoint;
    visualization_msgs::Marker goalPoint;
    visualization_msgs::Marker randomPoint;
    visualization_msgs::Marker rrtTreeMarker;
    visualization_msgs::Marker finalPath;

    initializeMarkers(sourcePoint, goalPoint, randomPoint, rrtTreeMarker, finalPath);

    ros::Subscriber sub_pp = n.subscribe("pp_request", 10, Callback_pp);
    ros::Subscriber sub_obst = n.subscribe("pp_obstacle", 10, Callback_obst);

    //Calculate the distance between previous goal point and current goal point, if it is more than 0.1m, processing the path planning algorithm

    while(ros::ok())
    {
      ros::spin();
    }

	return 1;
}
