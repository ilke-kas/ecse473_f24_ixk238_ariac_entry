#include "ros/ros.h"
#include "std_srvs/Trigger.h"
#include "std_srvs/SetBool.h"
#include "osrf_gear/Order.h"
#include "osrf_gear/Shipment.h"
#include "osrf_gear/Product.h"
#include "osrf_gear/GetMaterialLocations.h"
#include "osrf_gear/LogicalCameraImage.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "geometry_msgs/TransformStamped.h"
#include "ik_service/PoseIK.h"

// Declare the variable in this way where necessary in the code.
std_srvs::Trigger begin_comp;
// Declare an order vector
std::vector<osrf_gear::Order> order_vector;
// Declare the vector for the logical cameras (There are 3 types of them)
std::vector<osrf_gear::LogicalCameraImage> logical_cameras_bin(6);
std::vector<osrf_gear::LogicalCameraImage> logical_cameras_agv(2);
std::vector<osrf_gear::LogicalCameraImage> logical_cameras_quality(2);

// Declare the transformation buffer to maintain a list of transformations
tf2_ros::Buffer tfBuffer;

// Declare the found product pose vector
std::vector<geometry_msgs::Pose> found_product_pose;

//Declare an ik_service::PoseIK variable
ik_service::PoseIK ik_pose;

// This function is written to get the appropriate camera data according to given storage unit id
const osrf_gear::LogicalCameraImage* getCameraData(
    const std::string& storage_unit_id,
    const std::vector<osrf_gear::LogicalCameraImage>& logical_cameras_bin,
    const std::vector<osrf_gear::LogicalCameraImage>& logical_cameras_agv,
    const std::vector<osrf_gear::LogicalCameraImage>& logical_cameras_quality) {
    
    // Determine the camera type based on the storage unit ID prefix
    if (storage_unit_id.find("bin") == 0) {
        // It's a bin camera
        int bin_index = std::stoi(storage_unit_id.substr(3)) - 1;
        if (bin_index >= 0 && bin_index < logical_cameras_bin.size()) {
            return &logical_cameras_bin[bin_index];
        } else {
            ROS_WARN_STREAM("Invalid bin index for storage unit: " << storage_unit_id);
        }
    } else if (storage_unit_id.find("agv") == 0) {
        // It's an AGV camera
        int agv_index = std::stoi(storage_unit_id.substr(3)) - 1;
        if (agv_index >= 0 && agv_index < logical_cameras_agv.size()) {
            return &logical_cameras_agv[agv_index];
        } else {
            ROS_WARN_STREAM("Invalid AGV index for storage unit: " << storage_unit_id);
        }
    } else if (storage_unit_id.find("quality") == 0) {
        // It's a quality control camera
        int quality_index = std::stoi(storage_unit_id.substr(7)) - 1;
        if (quality_index >= 0 && quality_index < logical_cameras_quality.size()) {
            return &logical_cameras_quality[quality_index];
        } else {
            ROS_WARN_STREAM("Invalid quality control index for storage unit: " << storage_unit_id);
        }
    } else {
        ROS_WARN_STREAM("Unknown storage unit prefix: " << storage_unit_id);
    }
    
    return nullptr; // Return nullptr if no valid camera data found
}

//This function is written to debug what logical camera data looks like in the callbacks
void printlogicalCameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg) {
    ROS_INFO("Received LogicalCameraImage with %zu models", msg->models.size());

    for (const auto& model : msg->models) {
        // Print model type
        ROS_INFO("Model Type: %s", model.type.c_str());

        // Print position
        ROS_INFO("Position - x: %.2f, y: %.2f, z: %.2f",
                 model.pose.position.x,
                 model.pose.position.y,
                 model.pose.position.z);

        // Print orientation
        ROS_INFO("Orientation - x: %.2f, y: %.2f, z: %.2f, w: %.2f",
                 model.pose.orientation.x,
                 model.pose.orientation.y,
                 model.pose.orientation.z,
                 model.pose.orientation.w);
    }
}


// Callback function to subscribe to logical cameras
// agv Cameras
void  logicagv1CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
  //printlogicalCameraCallback(msg);
  logical_cameras_agv[0] = *msg;
}
void  logicagv2CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
 //printlogicalCameraCallback(msg);
  logical_cameras_agv[1] = *msg;
}
// Quality cameras callback
void logicQuality1CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
 //printlogicalCameraCallback(msg);
  logical_cameras_quality[0] = *msg;
}
void logicQuality2CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
 //printlogicalCameraCallback(msg);
  logical_cameras_quality[1] = *msg;
}
 //Bin Cameras
 void logicbin1CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
 // printlogicalCameraCallback(msg);
  logical_cameras_bin[0] = *msg;
}
 void logicbin2CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
 //printlogicalCameraCallback(msg);
  logical_cameras_bin[1] = *msg;
}
 void logicbin3CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
 //printlogicalCameraCallback(msg);
  logical_cameras_bin[2] = *msg;
}
 void logicbin4CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
//printlogicalCameraCallback(msg);
  logical_cameras_bin[3] = *msg;
}
 void logicbin5CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
 //printlogicalCameraCallback(msg);
  logical_cameras_bin[4] = *msg;
}
 void logicbin6CameraCallback(const osrf_gear::LogicalCameraImage::ConstPtr& msg){
//printlogicalCameraCallback(msg);
  logical_cameras_bin[5] = *msg;
}


//Callback function to subscribe the orders
void orderCallback(const osrf_gear::Order::ConstPtr& orders){
 // Add order to the end of the vector
  order_vector.push_back(*orders);
  // Print order id
  //ROS_INFO("Order ID: [%s]", orders->order_id.c_str());
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "start_competition_node");
  
  std_srvs::SetBool my_bool_var;
  my_bool_var.request.data = true;
  
  // "n" is the previously declared node handle.
  ros::NodeHandle n;

  // Instantiate a listener that listens to the tf and tf_static topics and to update the buffer.
  tf2_ros::TransformListener tfListener(tfBuffer);
  
  // clear the order vector
  order_vector.clear();

  //clear the camera vectors
  logical_cameras_bin.clear();
  logical_cameras_agv.clear();
  logical_cameras_quality.clear();

  // clear the found product pose 
  found_product_pose.clear();

  // resize the camera vectors
  logical_cameras_bin.resize(6);
  logical_cameras_agv.resize(2);
  logical_cameras_quality.resize(2);
  // subscribe to ariac/orders topic
  ros::Subscriber orders_subscriber = n.subscribe("/ariac/orders", 1000, orderCallback);
  
  //subscribe to all logical_camera topics. 
  ros::Subscriber logical_camera_subscriber_agv1 = n.subscribe("/ariac/logical_camera_agv1", 10, logicagv1CameraCallback);
  ros::Subscriber logical_camera_subscriber_agv2 = n.subscribe("/ariac/logical_camera_agv2", 10, logicagv2CameraCallback);
  ros::Subscriber logical_camera_subscriber_bin1 = n.subscribe("/ariac/logical_camera_bin1", 10, logicbin1CameraCallback);
  ros::Subscriber logical_camera_subscriber_bin2 = n.subscribe("/ariac/logical_camera_bin2", 10, logicbin2CameraCallback);
  ros::Subscriber logical_camera_subscriber_bin3 = n.subscribe("/ariac/logical_camera_bin3", 10, logicbin3CameraCallback);
  ros::Subscriber logical_camera_subscriber_bin4 = n.subscribe("/ariac/logical_camera_bin4", 10, logicbin4CameraCallback);
  ros::Subscriber logical_camera_subscriber_bin5 = n.subscribe("/ariac/logical_camera_bin5", 10, logicbin5CameraCallback);
  ros::Subscriber logical_camera_subscriber_bin6 = n.subscribe("/ariac/logical_camera_bin6", 10, logicbin6CameraCallback);
  ros::Subscriber logical_camera_subscriber_quality_control_sensor1 = n.subscribe("/ariac/quality_control_sensor_1", 10, logicQuality1CameraCallback);
  ros::Subscriber logical_camera_subscriber_quality_control_sensor2 = n.subscribe("/ariac/quality_control_sensor_2", 10, logicQuality2CameraCallback);


  // Create the service client.
  ros::ServiceClient begin_client = n.serviceClient<std_srvs::Trigger>("/ariac/start_competition");

  // Create client for pose_ik service
  ros::ServiceClient pose_ik_client = n.serviceClient<ik_service::PoseIK>("pose_ik");
  
  // Create service client in order to request the bin location of a specific material type 
  ros::ServiceClient request_bin_location = n.serviceClient<osrf_gear::GetMaterialLocations>("/ariac/material_locations");

  // Variable to capture service call success.
  int service_call_succeeded;

  // Call the Service
  service_call_succeeded = begin_client.call(begin_comp);

  int max_retry_count = 125;
  int retry_count = 0;
  // Call the Trigger service
  while((retry_count < max_retry_count) && (service_call_succeeded == 0)){
    // Call the Service
    service_call_succeeded = begin_client.call(begin_comp);
    if (service_call_succeeded == 0){
	    ROS_ERROR("Competition service call failed! Goodnes Gracious!!");
      ROS_ERROR("Retrying the call again!");
      retry_count= retry_count +1;
      ros::Duration(4.0).sleep();
    }
    else{
	  if(begin_comp.response.success)
    {
      ROS_INFO("Competition service called successfully: %s", begin_comp.response.message.c_str());
      break;
    }
    else{
	      ROS_WARN("Competition service returned failure: %s", begin_comp.response.message.c_str());
        break;
      }
    }
  }


  // Let ROS handle incoming messages 
  ros::spinOnce();
  ros::Duration(1.0).sleep(); // Allow some time for orders to be received

  // In order to iterate over the orders 
  while (ros::ok()) {
    ros::spinOnce(); // spinOnce for the callbacks

    // If the order_vector has a sıze greater than zero
    if (order_vector.size() > 0) {
      // get the current order from the vector
      osrf_gear::Order &curr_order = order_vector.back();
            
      // print order ID info to check in the output
      ROS_INFO("Order ID: %s", curr_order.order_id.c_str());


    // Iterate over each shipment in the order
    while (curr_order.shipments.size() > 0) {
      // get the current shipment
      osrf_gear::Shipment &curr_shipment = curr_order.shipments.back();
      // print the shipment type and agv_id info
      ROS_INFO("Shipment Type: %s, agv_id: %s", curr_shipment.shipment_type.c_str(), curr_shipment.agv_id.c_str());
                
      // Iterate over each product in the shipment
      while(curr_shipment.products.size() >0 ){
        // get the current product
       osrf_gear::Product &product = curr_shipment.products.back();
        ROS_INFO("-----------------PRODUCT----------------------");
        // print the type of the product and its pose
			  ROS_INFO("Product Type: %s", product.type.c_str());
			
				 
        // Now, according to the product type call the material_location service to find where that product type may found
        // Use GetMaterialLocations for that
        // It will return the storage_unit vector where the material may be found
        // Declare local get material location obj
        osrf_gear::GetMaterialLocations get_mat_location;
			
        // requested material type should be equal to the product type
        get_mat_location.request.material_type = product.type;
        
        // call the bin location service
        int bin_location_request_success = request_bin_location.call(get_mat_location);
        
        if(bin_location_request_success == 0){
          ROS_ERROR("Bin location service call  for this material failed! Goodnes Gracious!!");
        }
			  else{
				  // get the all storage units that may contain the specificed material type
				  for (const auto& storage_unit : get_mat_location.response.storage_units) {
					  // Print product type and its corresponding storage unit
					  ROS_INFO(" MATERIAL_LOCATION RESULTS: Product type: %s is in storage unit: %s", product.type.c_str(), storage_unit.unit_id.c_str());
					  // now, search through the logical_camera data for the bin specified by the material_location service for a product of the correct type 
					  if( storage_unit.unit_id != "belt"){
					    // get camera data 
						  const osrf_gear::LogicalCameraImage* camera_data = getCameraData(storage_unit.unit_id, logical_cameras_bin, logical_cameras_agv, logical_cameras_quality);
					 	  bool found_product = false;
						  for (const auto& model : camera_data->models) {
							  // Check if the model type matprint the pose of this tooches the product type we're looking for
							  ROS_INFO("Search through the logical_camera data for the bin specified by the material_location service");
							  if (model.type == product.type) {
							    // Log the product type, bin, and pose information
							    ROS_INFO_STREAM("Found product: " << product.type.c_str() 
									    << " in bin: " << storage_unit.unit_id.c_str() 
									    << " at position (x: " << model.pose.position.x 
									    << ", y: " << model.pose.position.y 
									    << ", z: " << model.pose.position.z 
									    << ") with orientation (x: " << model.pose.orientation.x 
									    << ", y: " << model.pose.orientation.y 
									    << ", z: " << model.pose.orientation.z 
									    << ", w: " << model.pose.orientation.w << ")");
							    
							    found_product = true;
                  // then let's store the pose of the found product so we can apply transformations on it later
                  found_product_pose.push_back(model.pose);
         
							    break; // Stop searching models if we found the product
							  }  
					    }
					    if (!found_product) {
						    ROS_WARN_STREAM("Product type " << product.type.c_str() << " not found in bin: " << storage_unit.unit_id.c_str());
					    }
            
               
                // if there is any found product, apply transformations
                while(found_product_pose.size() >0){

                   // get camera frame name in order to transform
                  std::string camera_frame_name = "logical_camera_"+storage_unit.unit_id+"_frame";
                  // get bin frame name in order to transform
                  std::string bin_frame_name = storage_unit.unit_id+ "_frame";

                  ROS_INFO("Camera frame name is %s", camera_frame_name.c_str());
                
                  geometry_msgs::PoseStamped part_pose, goal_pose;
                  part_pose.pose = found_product_pose.back();

                  
                  ROS_INFO("Part Pose in Camera Frame: Position -> x: %.2f, y: %.2f, z: %.2f, Orientation -> x: %.2f, y: %.2f, z: %.2f, w: %.2f",
                          part_pose.pose.position.x, part_pose.pose.position.y, part_pose.pose.position.z,
                          part_pose.pose.orientation.x, part_pose.pose.orientation.y,
                          part_pose.pose.orientation.z, part_pose.pose.orientation.w);

                  // Retrieve the transformation
                  geometry_msgs::TransformStamped tfStamped;
                  try {
                      tfStamped = tfBuffer.lookupTransform("arm1_base_link", camera_frame_name.c_str(),
                      ros::Time(0.0), ros::Duration(10.0));
                      ROS_INFO("Transform from [%s] to [%s]: Translation -> x=%.2f, y=%.2f, z=%.2f, Rotation -> x=%.2f, y=%.2f, z=%.2f, w=%.2f",
                      tfStamped.header.frame_id.c_str(), tfStamped.child_frame_id.c_str(),
                      tfStamped.transform.translation.x, tfStamped.transform.translation.y, tfStamped.transform.translation.z,
                      tfStamped.transform.rotation.x, tfStamped.transform.rotation.y, tfStamped.transform.rotation.z, tfStamped.transform.rotation.w);



                      // do the transfromation so the pose of the part with respect to the robot is known
                      tf2::doTransform(part_pose, goal_pose, tfStamped);

                      // Add height to the goal pose.
                      goal_pose.pose.position.z += 0.10; // 10 cm above the part

                      // Print the transformed goal pose
                      ROS_INFO("Transformed goal pose: Position -> x: %.2f, y: %.2f, z: %.2f, Orientation -> x: %.2f, y: %.2f, z: %.2f, w: %.2f",
                              goal_pose.pose.position.x, goal_pose.pose.position.y, goal_pose.pose.position.z,
                              goal_pose.pose.orientation.x, goal_pose.pose.orientation.y,
                              goal_pose.pose.orientation.z, goal_pose.pose.orientation.w);


                      // Set the request field of the ik_service::PoseIK variable equal to the goal pose
                      ik_pose.request.part_pose = goal_pose.pose;

                      // Update the client.call() to use the ik_service::PoseIK variable.
                      bool success = pose_ik_client.call(ik_pose);
                      if (success)
                      {
                      // Finally, update the ROS_INFO() messages to indicate that the client.call() returned request.num_sols solutions
                        ROS_INFO("Call to ik_service returned [%i] solutions", ik_pose.response.num_sols);
                        
                        // Add to the ROS_INFO() to show those set of joint angles
                        for(int i = 0 ; i < ik_pose.response.num_sols ; i++){
                          ROS_INFO("Solution Num: %d", i+1);
                          for(int j = 0 ; j < 6 ; j++){
                          ROS_INFO("Joint angle [%d]: %.4f ", j+1, ik_pose.response.joint_solutions[i].joint_angles[j]);
                          }
                        
                        

                      }
                      //Remove the processed product from the queue when its is completd
			                found_product_pose.pop_back();
                      
                      }
                      else
                      {
                        if(!success){
                          //Remove the processed product from the queue when its is completd
                          found_product_pose.pop_back();
                          // Finally, update the ROS_INFO() messages to indicate that the client.call() failed
                            ROS_INFO("ik_service could not find a solution for this transformation: Transformed goal pose: Position -> x: %.2f, y: %.2f, z: %.2f, Orientation -> x: %.2f, y: %.2f, z: %.2f, w: %.2f",
                              goal_pose.pose.position.x, goal_pose.pose.position.y, goal_pose.pose.position.z,
                              goal_pose.pose.orientation.x, goal_pose.pose.orientation.y,
                              goal_pose.pose.orientation.z, goal_pose.pose.orientation.w);
                        }
                        else{
                          //Remove the processed product from the queue when its is completd
                          found_product_pose.pop_back();
                          // Finally, update the ROS_INFO() messages to indicate that the client.call() failed
                            ROS_ERROR("Failed to call service ik_service");

                        }
                        
                        
                      }

                    } catch (tf2::TransformException &ex) {
                      ROS_ERROR("%s", ex.what());
                    }
                }
					  }
					
				  }	
				
			    }
			    //Remove the processed product from the queue when its is completd
			    curr_shipment.products.pop_back();
			    ROS_INFO("-------------------------------------------");
        }

        // Remove the processed shipment from the queue when it is completd
        curr_order.shipments.pop_back();
      }

      // Remove the processed order from the queue when it is completed
      order_vector.pop_back();
    } 
  else {
          // Check for new orders every second
          ros::Duration(1.0).sleep(); 
  } 
  }

  ros::spin(); 
  return 0;
}
