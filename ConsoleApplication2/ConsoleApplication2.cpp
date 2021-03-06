// ConsoleApplication2.cpp : define o ponto de entrada para o aplicativo do console.
//

#include "stdafx.h"
#include <pcl/conversions.h>

#include <pcl/common/common.h>

#include <pcl/point_cloud.h>
//#include <pcl/point_types.h>
#include <pcl/features/normal_3d.h>

#include <pcl/search/impl/search.hpp>
#ifndef PCL_NO_PRECOMPILE
#include <pcl/impl/instantiate.hpp>
#include <pcl/point_types.h>
PCL_INSTANTIATE(Search, PCL_POINT_TYPES)
#endif // PCL_NO_PRECOMPILE

//Visualization
#include <thread>
#include <chrono>
#include <iostream>
#include <pcl/visualization/cloud_viewer.h>
//#include <pcl_conversions/pcl_conversions.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/common/common_headers.h>
#include <boost/thread/thread.hpp>


//Outlier removal
#include <pcl/filters/statistical_outlier_removal.h>

//rotation
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/console/parse.h>
#include <pcl/common/transforms.h>


//Find 3d correspondence
#include <pcl/correspondence.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/features/shot_omp.h>
#include <pcl/features/board.h>
//#include <pcl/filters/uniform_sampling.h>
#include <pcl/keypoints/uniform_sampling.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/recognition/cg/hough_3d.h>
#include <pcl/recognition/cg/geometric_consistency.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/kdtree/impl/kdtree_flann.hpp>



#include <pcl/filters/extract_indices.h>
#include <pcl/point_types.h>
#include <pcl/sample_consensus/ransac.h>
#include <pcl/sample_consensus/sac_model_plane.h>
#include <pcl/sample_consensus/sac_model_sphere.h>


//segmentation part of the code
#include <vector>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/search/search.h>
#include <pcl/search/kdtree.h>
#include <pcl/features/normal_3d.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/filters/passthrough.h>
#include <pcl/segmentation/region_growing.h>


// Read configuration from file.
#include <fstream>
#include <string>
#include <windows.h>


//Verficação de consistencia
#include <pcl/recognition/hv/hv_go.h>
#include <pcl/registration/icp.h>

//Read Ply files
#include<pcl/io/ply_io.h>

typedef pcl::PointXYZRGBA PointType;
typedef pcl::Normal NormalType;
typedef pcl::ReferenceFrame RFType;
typedef pcl::SHOT352 DescriptorType;

using CloudCpt = pcl::PointCloud<PointType>;


std::string workingdir()
{
	char buf[256];
	GetCurrentDirectoryA(256, buf);
	return std::string(buf) + '\\';
}

pcl::PointCloud<PointType>::Ptr rotateX(pcl::PointCloud<PointType>::Ptr cloudf, float theta)
{
	/* Reminder: how transformation matrices work :

	|-------> This column is the translation
	| 1 0 0 x |  \
	| 0 1 0 y |   }-> The identity 3x3 matrix (no rotation) on the left
	| 0 0 1 z |  /
	| 0 0 0 1 |    -> We do not use this line (and it has to stay 0,0,0,1)


	*/

	// Define a rotation matrix (see https://en.wikipedia.org/wiki/Rotation_matrix)
	//float theta = -M_PI/2; // The angle of rotation in radians


	/*  METHOD #2: Using a Affine3f
	This method is easier and less error prone
	*/
	Eigen::Affine3f transform_2 = Eigen::Affine3f::Identity();

	// Define a translation of 2.5 meters on the x axis.
	transform_2.translation() << 2.5, 0.0, 0.0;

	// The same rotation matrix as before; theta radians around Z axis
	transform_2.rotate(Eigen::AngleAxisf(theta, Eigen::Vector3f::UnitX()));

	// Print the transformation
	printf("\nMethod #2: using an Affine3f\n");
	std::cout << transform_2.matrix() << std::endl;

	// Executing the transformation
	pcl::PointCloud<PointType>::Ptr transformed_cloud(new pcl::PointCloud<PointType>());
	// You can either apply transform_1 or transform_2; they are the same
	pcl::transformPointCloud(*cloudf, *transformed_cloud, transform_2);

	return transformed_cloud;
}



//-----------------------------------
//Algorithm params
bool show_keypoints_(false);
bool show_correspondences_(false);
bool use_cloud_resolution_(false);
bool use_hough_(false);

float model_ss_(0.002f);
float scene_ss_(0.002f);
float rf_rad_(0.005f);
float descr_rad_(0.005f);
float cg_size_(0.01f);
float cg_thresh_(5.0f);

//Parametros da verificaçao de consistencia
int icp_max_iter_(5);
float icp_corr_distance_(0.2f);
float hv_resolution_(0.05f);
float hv_occupancy_grid_resolution_(0.01f);
float hv_clutter_reg_(5.0f);
float hv_inlier_th_(0.05f);
float hv_occlusion_th_(0.01f);
float hv_rad_clutter_(0.03f);
float hv_regularizer_(3.0f);
float hv_rad_normals_(0.05);
bool hv_detect_clutter_(true);


/*
float model_ss_(0.01f);
float scene_ss_(0.03f);
float rf_rad_(0.015f);
float descr_rad_(0.02f);
float cg_size_(0.01f);
float cg_thresh_(1.0f);
*/
//-----------------------------------


// --------------------- Visualization ------------------------------
int user_data;

void
viewerOneOff(pcl::visualization::PCLVisualizer& viewer)
{
	viewer.setBackgroundColor(1.0, 0.5, 1.0);
	pcl::PointXYZ o;
	o.x = 1.0;
	o.y = 0;
	o.z = 0;
	viewer.addSphere(o, 0.25, "sphere", 0);
	std::cout << "i only run once" << std::endl;

}

void
viewerPsycho(pcl::visualization::PCLVisualizer& viewer)
{
	static unsigned count = 0;
	std::stringstream ss;
	ss << "Once per viewer loop: " << count++;
	viewer.removeShape("text", 0);
	viewer.addText(ss.str(), 200, 300, "text", 0);

	//FIXME: possible race condition here:
	user_data++;
}

// --------------------------------END OF CLOUD VIEW-------------------

pcl::visualization::PCLVisualizer::Ptr rgbVis(pcl::PointCloud<PointType>::ConstPtr cloud)
{
	// --------------------------------------------
	// -----Open 3D viewer and add point cloud-----
	// --------------------------------------------
	pcl::visualization::PCLVisualizer::Ptr viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
	viewer->setBackgroundColor(0, 0, 0);
	pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGBA> rgb(cloud);
	viewer->addPointCloud<pcl::PointXYZRGBA>(cloud, rgb, "sample cloud");
	viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 3, "sample cloud");
	viewer->addCoordinateSystem(1.0);
	viewer->initCameraParameters();
	return (viewer);
}


unsigned int text_id = 0;
void keyboardEventOccurred(const pcl::visualization::KeyboardEvent &event,
	void* viewer_void)
{
	pcl::visualization::PCLVisualizer *viewer = static_cast<pcl::visualization::PCLVisualizer *> (viewer_void);
	if (event.getKeySym() == "r" && event.keyDown())
	{
		std::cout << "r was pressed => removing all text" << std::endl;

		char str[512];
		for (unsigned int i = 0; i < text_id; ++i)
		{
			sprintf(str, "text#%03d", i);
			viewer->removeShape(str);
		}
		text_id = 0;
	}
}

void mouseEventOccurred(const pcl::visualization::MouseEvent &event,
	void* viewer_void)
{
	pcl::visualization::PCLVisualizer *viewer = static_cast<pcl::visualization::PCLVisualizer *> (viewer_void);
	if (event.getButton() == pcl::visualization::MouseEvent::LeftButton &&
		event.getType() == pcl::visualization::MouseEvent::MouseButtonRelease)
	{
		std::cout << "Left mouse button released at position (" << event.getX() << ", " << event.getY() << ")" << std::endl;

		char str[512];
		sprintf(str, "text#%03d", text_id++);
		viewer->addText("clicked here", event.getX(), event.getY(), str);
	}
}

pcl::visualization::PCLVisualizer::Ptr interactionCustomizationVis()
{
	pcl::visualization::PCLVisualizer::Ptr viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
	viewer->setBackgroundColor(0, 0, 0);
	viewer->addCoordinateSystem(1.0);

	viewer->registerKeyboardCallback(keyboardEventOccurred, (void*)viewer.get());
	viewer->registerMouseCallback(mouseEventOccurred, (void*)viewer.get());

	return (viewer);
}



// --------------------------------END OF PCL VISUALIZER-------------------


// ----------------------------- fUNCTION to compute distance ---------------------

float compute(CloudCpt &cloud_a, CloudCpt &cloud_b, bool imprime)
{
	// Estimate
	//TicToc tt;
	//tt.tic();

	//print_highlight(stderr, "Computing ");

	// compare A to B
	pcl::search::KdTree<PointType> tree_b;
	tree_b.setInputCloud(cloud_b.makeShared());
	float max_dist_a = -std::numeric_limits<float>::max();
	for (const auto &point : cloud_a.points)
	{
		std::vector<int> indices(1);
		std::vector<float> sqr_distances(1);

		tree_b.nearestKSearch(point, 1, indices, sqr_distances);
		if (sqr_distances[0] > max_dist_a)
			max_dist_a = sqr_distances[0];
	}

	// compare B to A
	pcl::search::KdTree<PointType> tree_a;
	tree_a.setInputCloud(cloud_a.makeShared());
	float max_dist_b = -std::numeric_limits<float>::max();
	for (const auto &point : cloud_b.points)
	{
		std::vector<int> indices(1);
		std::vector<float> sqr_distances(1);

		tree_a.nearestKSearch(point, 1, indices, sqr_distances);
		if (sqr_distances[0] > max_dist_b)
			max_dist_b = sqr_distances[0];
	}

	max_dist_a = std::sqrt(max_dist_a);
	max_dist_b = std::sqrt(max_dist_b);

	float dist = std::min(max_dist_a, max_dist_b);
	//float dist = max_dist_b;

	if (imprime)
	{
		std::cout << "[done, ";
		std::cout << "A->B: " << max_dist_a;
		std::cout << ", B->A: " << max_dist_b;
		std::cout << ", Hausdorff Distance: " << dist;
		std::cout << " ]" << std::endl;
	}

	return dist;
}




int main(int argc, char** argv)
{

	

	printf("'Started! \n");

	//Diretório de execução do código
	cout << "my directory is " << workingdir() << "\n";

	std::string param = "rec1.ply";

	std::string objname = "rec2.ply";

	int cont;
	int tipo = 0;

	if (argc == 2)
	{
		tipo = atoi(argv[1]);
		cout << "Funcao: " << tipo << "\n";
	}

	if (argc == 3)
	{
		tipo = atoi(argv[1]);
		param = argv[2];
		cout << "Funcao: " << tipo << "\n";
	}
	if (argc == 4)
	{
		tipo = atoi(argv[1]);
		param = argv[2];
		objname = argv[3];
		cout << "Funcao: " << tipo << "\n";
	}

	//Verifica se é para converter de ply para xyz ou de xyz para PLY

	if (tipo == 0)
	{
		// ==========================================================================================
		//Carrega os arquivos

		pcl::PointCloud<PointType>::Ptr model(new pcl::PointCloud<PointType>());
		pcl::PointCloud<PointType>::Ptr scene(new pcl::PointCloud<PointType>());


		//ply file reader 
		pcl::PLYReader Reader;

		if (Reader.read(workingdir() + param, *scene) == -1)
		{
			PCL_ERROR("Couldn't read file ");
			return (-1);
		}

		std::cout << "Loaded "
			<< scene->width * scene->height
			<< " data points with the following fields: "
			<< std::endl;

		if (Reader.read(workingdir() + objname, *model) == -1)
		{
			PCL_ERROR("Couldn't read file ");
			return (-1);
		}

		std::cout << "Loaded model "
			<< model->width * model->height
			<< " data points from test_pcd.pcd with the following fields: "
			<< model->points.size()
			<< std::endl;

		std::vector<int> indicesEcldMdl;
		pcl::removeNaNFromPointCloud(*model, *model, indicesEcldMdl);
		std::cout << "Filtered size: " << model->points.size() << std::endl;



		// ******************************************************************************************

		// Remoove header from file 1

		// pcl::PointCloud<pcl::PointXYZ> outputXYZ;
		pcl::PointCloud<pcl::PointXYZ>::Ptr outputXYZ(new pcl::PointCloud<pcl::PointXYZ>());
		pcl::copyPointCloud(*scene, *outputXYZ);

		pcl::io::savePLYFileASCII(workingdir() + "nvm1_tmp.ply", *outputXYZ);
		//pcl::io::savePLYFileBinary(workingdir() + "results\\match.ply", *outputXYZ);


		ifstream file;
		ofstream outfile;
		std::string lines;
		std::string endline = "end_header";
		outfile.open("nvm1.xyz");
		file.open("nvm1_tmp.ply");

		int cp = 0;
		while (getline(file, lines))
		{
			if (cp == 1)
			{
				outfile << lines << endl;
			}
			/*else
			{
				outfile << lines << endl;
			}*/
			if (lines.compare(endline) == 0)
			{
				cp = 1;
			}
		}
		outfile.close();
		file.close();


		// ******************************************************************************************

		// Remoove header from file 2
		// pcl::PointCloud<pcl::PointXYZ> outputXYZ;
		pcl::PointCloud<pcl::PointXYZ>::Ptr outputXYZ2(new pcl::PointCloud<pcl::PointXYZ>());
		pcl::copyPointCloud(*model, *outputXYZ2);

		pcl::io::savePLYFileASCII(workingdir() + "nvm2_tmp.ply", *outputXYZ2);
		//pcl::io::savePLYFileBinary(workingdir() + "results\\match.ply", *outputXYZ);


		ifstream file2;
		ofstream outfile2;

		outfile2.open("nvm2.xyz");
		file2.open("nvm2_tmp.ply");

		cp = 0;
		while (getline(file2, lines))
		{
			if (cp == 1)
			{
				outfile2 << lines << endl;
			}
			/*else
			{
				outfile << lines << endl;
			}*/
			if (lines.compare(endline) == 0)
			{
				cp = 1;
			}
		}
		outfile2.close();
		file2.close();

		// ---------------------------------------------------------------
		//                Transform point cloud to generate normal reference
		// ---------------------------------------------------------------

		pcl::PointCloud<pcl::PointXYZ>::Ptr transformed_cloud(new pcl::PointCloud<pcl::PointXYZ>());

		Eigen::Affine3f transform_2 = Eigen::Affine3f::Identity();

		// Define a translation of 2.5 meters on the x axis.
		transform_2.translation() << 0.0, 0.0, 1.0;

		pcl::transformPointCloud(*outputXYZ2, *transformed_cloud, transform_2);




		// Remoove header from file 3
		pcl::io::savePLYFileASCII(workingdir() + "nvm3_tmp.ply", *transformed_cloud);

		ifstream file3;
		ofstream outfile3;

		outfile3.open("nvm3.xyz");
		file3.open("nvm3_tmp.ply");

		cp = 0;
		while (getline(file3, lines))
		{
			if (cp == 1)
			{
				outfile3 << lines << endl;
			}
			if (lines.compare(endline) == 0)
			{
				cp = 1;
			}
		}
		outfile3.close();
		file3.close();

		// --------------------------------------------------------------
	} else if (tipo == 1)
	{
		std::cout << "-- " ;
		// Tipo 1 converte de xyz para ply
	
		if (argc == 2)
		{
			param = "result.xyz";
			objname = "result.ply";
		}

		if (argc == 3)
		{
			objname = "result.ply";
		}
		
		//---------------------
		// conta numero de linhas do arquivo de entrada
		//---------------

		double max  = -999999;
		double min  =  999999;

		double meanv = 0;
		
		double std_dev = 0;


		
		ifstream file;
		std::string lines;
		file.open(param);

		int numLines = 0;
		getline(file, lines);

		std::cout << " --  " << std::endl;
		while (getline(file, lines))
		{
			numLines = numLines + 1;

			std::istringstream iss(lines);
			std::string s;


			//Cordinates
			iss >> s;
			iss >> s;
			iss >> s;

			//normal
			iss >> s;
			iss >> s;
			iss >> s;

			std::string scalar;

			// Scalar
			iss >> scalar;
			iss >> s;

			double scValue = std::stof(scalar);

			if ( scValue > max )
				max = scValue;

			if ( scValue < min )
				min = scValue;

			meanv = meanv + abs(scValue);

		}

		meanv = meanv / numLines;

		file.close();

		std::cout << "Num lines: " << numLines << std::endl;
		std::cout << "Max: " << max << " Min: " << min << " Mean: " << meanv ;

		//Repete para estimar desvio padrão

		file.open(param);

		getline(file, lines);

		while (getline(file, lines))
		{

			std::istringstream iss(lines);
			std::string s;


			//Cordinates
			iss >> s;
			iss >> s;
			iss >> s;

			//normal
			iss >> s;
			iss >> s;
			iss >> s;

			std::string scalar;

			// Scalar
			iss >> scalar;
			iss >> s;

			double scValue = std::stof(scalar);

			std_dev = (scValue - meanv) * (scValue - meanv);

		}


		file.close();

		std_dev = std_dev / (numLines - 1);

		std_dev = sqrt(std_dev);

		std::cout << " Std_dev: " << std_dev << std::endl;
		


		// ---------------------------------  -----------------------
		//    Escreve cabeçario do arquivo
		// ---------------------------------  -----------------------



		ifstream file2;
		ofstream outfile2;

		outfile2.open(objname);
		file2.open(param);

		outfile2 << "ply" << endl;
		outfile2 << "format ascii 1.0" << endl;
		outfile2 << "comment created by SMART3D" << endl;
		outfile2 << "obj_info Generated by SMART!" << endl;
		outfile2 << "element vertex " << numLines-3 <<endl;
		outfile2 << "property float x" << endl;
		outfile2 << "property float y" << endl;
		outfile2 << "property float z" << endl;

		outfile2 << "property uchar red" << endl;
		outfile2 << "property uchar green" << endl;
		outfile2 << "property uchar blue" << endl;

		outfile2 << "property float nx" << endl;
		outfile2 << "property float ny" << endl;
		outfile2 << "property float ny" << endl;
					

		outfile2 << "property float scalar" << endl;
		outfile2 << "property float scalar_sig" << endl;
		outfile2 << "end_header" << endl;
			



		// ---------------------------------  -----------------------
		//    Re-escreve os dados
		// ---------------------------------  -----------------------

		//Descarta primeira linha
		getline(file2, lines);

		int numParsed = 1;

		while (getline(file2, lines))
		{

			std::istringstream iss(lines);			
			std::string s; 


			//Cordinates
			iss >> s;
			outfile2 << s <<" ";

			iss >> s;
			outfile2 << s << " ";

			iss >> s;
			outfile2 << s << " ";

			std::string nx;
			std::string ny;
			std::string nz;

			//normal
			iss >> nx;
			iss >> ny;
			iss >> nz;
			
			std::string scalar;
			std::string scalarDiff;

			// Scalar
			iss >> scalar;
			iss >> scalarDiff;

			double scValue = std::stof(scalar);

			//color satura em 95% do intervalo de confiança
			if (abs(scValue) < abs(2 * std_dev + meanv))
			{
				//outfile2 << "255 255 255 ";

				double fator = 255 / abs(2 * std_dev + meanv);

				int auxv = 255 - abs(scValue) * fator;
				
				if (scValue > 0)
				{
					outfile2 << "150 " << auxv << " " << auxv << " ";
				}
				else
				{
					outfile2  << auxv << " 150 " << auxv << " ";
				}

			}
			else
			{
				if (scValue > 0)
				{
					outfile2 << "150 0 0 ";
				}
				else
				{
					outfile2 << "0 150 0 ";
				}

			}




			outfile2 << nx << " ";
			outfile2 << ny << " ";
			outfile2 << nz << " ";

			outfile2 << scalar << " ";
			outfile2 << scalarDiff << endl;

			numParsed++;

			if (numParsed > (numLines - 3))
				break;

		}

		// ---------------------------------  -----------------------
		//    Fim Fecha arquivos
		// ---------------------------------  -----------------------


		outfile2.close();
		file2.close();

	}
	else if (tipo == 3)
	{
		// Tipo 1 converte de xyz para ply
		//---------------------
		// conta numero de linhas do arquivo de entrada
		//---------------
		
	}
	else
	{
		printf("'Function not implemented yet!");
 }


	printf("'End!");
	return 0;
}

