#include <cstdio> // Para utilizar archivos: fopen, fread, fwrite, gets...
#include <iostream> // Funciones cout y cin
#include <filesystem>
#include <experimental\filesystem>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2\reg\map.hpp>
#include "opencv2\reg\mappergradshift.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include "Funciones varias.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace cv;
using namespace experimental::filesystem::v1;



using namespace cv;
using namespace std;

Mat AlineaImg(Mat& img_base, Mat& img_movida)
{

	// Ajustarlas a 8 bits y un canal.
	img_base /= 255;
	img_base.convertTo(img_base, CV_8UC1); // Le pongo un canal para el gris.
	img_movida /= 255;
	img_movida.convertTo(img_movida, CV_8UC1); // Le pongo un canal para el gris.

								 //imshow("Image 1", im1);
								 //imshow("Image 2", im2);
								 //waitKey(0);

								 // Definir el tipo de movimiento
	//const int warp_mode = MOTION_TRANSLATION;
	const int warp_mode = MOTION_HOMOGRAPHY;


	// Establecer la matriz de la deformaci�n 2x3 or 3x3 seg�n el movimiento.
	Mat warp_matrix;

	// Inicializa la matriz de la deformaci�n como identidad
	if (warp_mode == MOTION_HOMOGRAPHY)
		warp_matrix = Mat::eye(3, 3, CV_32F);
	else
		warp_matrix = Mat::eye(2, 3, CV_32F);

	// Especificar el n�mero de iteraciones
	int number_of_iterations = 5000;

	// Especificar el umbral del incremento
	// en el coeficiente de correlaci�n entre dos iteraciones
	double termination_eps = 1e-10;

	// Definir el criterio de terminaci�n
	TermCriteria criteria(TermCriteria::COUNT + TermCriteria::EPS, number_of_iterations, termination_eps);

	// Ejecutar el algoritmo ECC. Los resultados se almacenan en warp_matrix.
	findTransformECC(
		img_base,
		img_movida,
		warp_matrix,
		warp_mode,
		criteria
	);

	cv::FileStorage guardar("Matriz_Traslacion.yml", cv::FileStorage::WRITE);
	guardar << "matriz inicial" << warp_matrix;
	guardar.release();

	// Para almacenar la imagen alineada
	Mat img_alineada;

	if (warp_mode != MOTION_HOMOGRAPHY)
		// Funci�n warpAffine para traslaciones, transformaciones euclideas y afines.
		warpAffine(img_movida, img_alineada, warp_matrix, img_base.size(), INTER_LINEAR + WARP_INVERSE_MAP);
	else
		// Funci�n warpPerspective para homograf�as
		warpPerspective(img_movida, img_alineada, warp_matrix, img_base.size(), INTER_LINEAR + WARP_INVERSE_MAP);

	// Mostrar el resultado final
	imshow("Image 1", img_base);
	imshow("Image 2", img_movida);
	imshow("Image 2 Alineada", img_alineada);
	waitKey(0);
	
	return img_alineada;
}




void GuardarMatCamara(Mat& matriz, string ruta)
{
	cv::FileStorage guardar(ruta, cv::FileStorage::WRITE);
	guardar << "Distancia focal X" << matriz.at<double>(0, 0);
	guardar << "Distancia focal Y" << matriz.at<double>(1, 1);
	guardar << "Punto principal X" << matriz.at<double>(0, 2);
	guardar << "Punto principal Y" << matriz.at<double>(1, 2);
	guardar.release();
}

void GuardarMatDistorsion(Mat& matriz, string ruta)
{
	cv::FileStorage guardar(ruta, cv::FileStorage::WRITE);
	guardar << "k1" << matriz.at<double>(0, 0);
	guardar << "k2" << matriz.at<double>(0, 1);
	guardar << "p1" << matriz.at<double>(0, 2);
	guardar << "p2" << matriz.at<double>(0, 3);
	guardar << "k3" << matriz.at<double>(0, 4);
	if (matriz.cols - 5 > 0)
	{
		guardar << "k4" << matriz.at<double>(0, 5);
	}
	if (matriz.cols - 6 > 0)
	{
		guardar << "k5" << matriz.at<double>(0, 6);
	}
	if (matriz.cols - 7 > 0)
	{
		guardar << "k6" << matriz.at<double>(0, 7);
	}
	guardar.release();
}

void GuardarMatFisica(vector<double> & vector, string ruta)
{
	cv::FileStorage guardar(ruta, cv::FileStorage::WRITE);
	guardar << "Distancia focal" << vector[0];
	guardar << "Relacion de aspecto" << vector[1];
	guardar << "Punto principal X" << vector[2];
	guardar << "Punto principal Y" << vector[3];
	guardar << "Field of View X" << vector[4];
	guardar << "Field of View Y" << vector[5];
	guardar << "Error de reproyeccion" << vector[6];
	guardar.release();
}




Mat LeerMatCamara(string ruta_ext)
{
	Mat matrizcam = Mat(Size(3, 3), CV_64F); // Matriz intr�nseca de la c�mara
	FileStorage leer_matcam(ruta_ext, FileStorage::READ);
	leer_matcam["Distancia focal X"] >> matrizcam.at<double>(0, 0);
	leer_matcam["Distancia focal Y"] >> matrizcam.at<double>(1, 1);
	leer_matcam["Punto principal X"] >> matrizcam.at<double>(0, 2);
	leer_matcam["Punto principal Y"] >> matrizcam.at<double>(1, 2);
	matrizcam.at<double>(0, 1) = 0.0;
	matrizcam.at<double>(1, 0) = 0.0;
	matrizcam.at<double>(2, 0) = 0.0;
	matrizcam.at<double>(2, 1) = 0.0;
	matrizcam.at<double>(2, 2) = 1.0;
	return matrizcam;
}

Mat LeerMatDistorsion(string ruta_ext, int max_k)
{
	Mat distcoef = Mat(Size(2 + max_k, 1), CV_64F); // Matriz de coeficientes de distorsi�n de la c�mara

	FileStorage leer_matdist(ruta_ext, FileStorage::READ);
	leer_matdist["k1"] >> distcoef.at<double>(0, 0);
	leer_matdist["k2"] >> distcoef.at<double>(0, 1);
	leer_matdist["p1"] >> distcoef.at<double>(0, 2);
	leer_matdist["p2"] >> distcoef.at<double>(0, 3);

	if (max_k + 2 > 4)
	{
		leer_matdist["k3"] >> distcoef.at<double>(0, 4);

	}
	if (max_k + 2 > 5)
	{
		leer_matdist["k4"] >> distcoef.at<double>(0, 5);

	}
	if (max_k + 2 > 6)
	{
		leer_matdist["k5"] >> distcoef.at<double>(0, 6);

	}
	if (max_k + 2 > 7)
	{
		leer_matdist["k6"] >> distcoef.at<double>(0, 7);

	}
	return distcoef;
}




void CalibraRGB(string ruta_carpeta_entrada, string& banda_extension, int& num_k, string ruta_salida_deteccionesquina)
{
	/// CALIBRACI�N DE UN SET DE IM�GENES PARA BANDA RGB
	/// **********************************************************************************************************************************************************************

	path ruta(ruta_carpeta_entrada);
	int num_img = 0;
	vector<vector<cv::Point2f>> coord_imagen; // Vector de vectores de puntos detectados en las im�genes
	vector<vector<cv::Point3f>> coord_obj; // Vector de vectores de puntos en coordenadas locales (se repiten siempre en cada imagen)
	cv::Mat matrizcam = Mat(Size(3, 3), CV_64F); // Matriz intr�nseca de la c�mara
	//matrizcam.at<double>(0, 0) = 1.0; // Para mantener el ratio de aspecto. Se dispara la fx.
	//matrizcam.at<double>(1, 1) = 1.0;

	cv::Mat distcoef = Mat(Size(8, 1), CV_64F); // Matriz de coeficientes de distorsi�n de la c�mara
	vector<cv::Mat> rotmat; // Matriz extr�nseca de rotaciones de la c�mara
	vector<cv::Mat> trasmat; // Matriz extr�nseca de traslaciones de la c�mara

	for (directory_entry p : recursive_directory_iterator(ruta)) // iteraci�n en carpetas y subcarpetas de la ruta
	{
		path ruta_archivo = p; // Cada archivo o subcarpeta localizado se utiliza como clase path
		string direc = ruta_archivo.string(); // convertir la ruta de clase path a clase string
		string nombre = ruta_archivo.filename().string(); // convertir el nombre del archivo de clase path a string

		if (nombre.size() > 8) // Si el nombre es grande de 8 caracteres significa que es una foto (evita carpetas).
		{
			if (nombre.compare(nombre.size() - 7, 7, banda_extension) == 0) // Encuentra imagen banda buscada
			{
				num_img += 1; // cada vez que entra significa que encuentra una imagen de la banda que quiere

				Mat imagen = cv::imread(direc, CV_LOAD_IMAGE_COLOR); // Para abrir im�genes de 16 bits por p�xel CV_LOAD_IMAGE_ANYDEPTH. Para abrir BGR -> CV_LOAD_IMAGE_COLOR
				
				cv::Size tamano(12, 8); // n�mero de esquinas a localizar
				vector<cv::Point2f> esquinas; // coordenadas de las esquinas detectadas en la imagen

				bool loc = cv::findChessboardCorners(imagen, tamano, esquinas, CV_CALIB_CB_ADAPTIVE_THRESH);
				if (loc == true) // si se localizan bien todas las esquinas es cuando se introduce dentro del set de calibraci�n
				{
					vector<cv::Point3f> obj; // vector de esquinas para la imagen de estudio en coordenadas locales
					for (int j = 1; j <= 8; j++)
					{
						for (int i = 1; i <= 12; i++)
						{
							obj.push_back({ (float(i) - 1.0f) * 30.0f,(float(j) - 1.0f) * 30.0f,0.0f }); // Tablero 30 x 30
						}
					}
					coord_obj.push_back(obj); // introduces las esquinas en coordenadas locales del tablero en el vector general
					coord_imagen.push_back(esquinas); // introduces las esquinas detectadas en coordenadas de la imagen en el vector general

					cvtColor(imagen, imagen, CV_BGR2GRAY);
					
					cv::cornerSubPix(imagen, esquinas, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, 0.1)); // funci�n para la detecci�n precisa de las esquinas
					
					//cv::cvtColor(imagen, imagen, CV_GRAY2RGB); // Se transforma la imagen a 3 canales de color para ponerle la detecci�n de esquinas en color.
					//cv::drawChessboardCorners(imagen, tamano, esquinas, loc); // Dibujo de las esquinas detectadas en colores
					

					//cv::imwrite(ruta_salida_deteccionesquina + std::to_string(num_img) + banda_extension, imagen);
				}

			}
		}

	}

	double error_repro;
	if (num_k == 3)
	{
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(4608, 3456), matrizcam, distcoef, rotmat, trasmat); // Calibraci�n de la c�mara
	}
	else if (num_k == 2)
	{
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(4608, 3456), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_FIX_K3); // Calibraci�n de la c�mara
	}															  
	else if (num_k == 4)										  
	{															  
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(4608, 3456), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_RATIONAL_MODEL | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6); // Calibraci�n de la c�mara
	}															   
	else if (num_k == 5)										   
	{															   
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(4608, 3456), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_RATIONAL_MODEL | CV_CALIB_FIX_K6); // Calibraci�n de la c�mara
	}															
	else if (num_k == 6)										
	{															
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(4608, 3456), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_RATIONAL_MODEL); // Calibraci�n de la c�mara
	}


	double ancho = 6.17472; // par�metros obtenidos mediante la multiplicaci�n del tama�o del p�xel (3.75 micras para monocrom�ticas) y de la resoluci�n de la imagen 
	double alto = 4.63104; // especifican el ancho y el alto del tama�o del sensor. Probablemente por el tipo de obturador CCD mono y CMOS RGB.
	double fov_x;
	double fov_y;
	double dist_focal;
	cv::Point2d punto_prin;
	double ratio_aspecto;

	cv::calibrationMatrixValues(matrizcam, Size(4608, 3456), ancho, alto, fov_x, fov_y, dist_focal, punto_prin, ratio_aspecto); // Par�metros f�sicos de la c�mara

	vector<double>matrizfisica; // Vector para obtener la salida de par�metros
	matrizfisica.push_back(dist_focal);
	matrizfisica.push_back(ratio_aspecto);
	matrizfisica.push_back(punto_prin.x);
	matrizfisica.push_back(punto_prin.y);
	matrizfisica.push_back(fov_x);
	matrizfisica.push_back(fov_y);
	matrizfisica.push_back(error_repro);


	string nombre_salida = banda_extension;
	for (int i = 0; i <= 3; i++) // quitarle el .JPG
	{
		nombre_salida.pop_back();
	}

	nombre_salida.append("_k");
	nombre_salida.append(to_string(num_k));
	nombre_salida.append(".yml");

	string salida = "Matriz_Fisica_";
	salida.append(nombre_salida);
	GuardarMatFisica(matrizfisica, salida);

	salida = "Matriz_Camara_";
	salida.append(nombre_salida);
	GuardarMatCamara(matrizcam, salida);

	salida = "Matriz_Distorsion_";
	salida.append(nombre_salida);
	GuardarMatDistorsion(distcoef, salida);

	/*
	// Correcci�n de la distorsi�n en im�genes de prueba
	cv::Mat prueba = cv::imread("pruebaRGB.JPG", CV_LOAD_IMAGE_ANYDEPTH); // Para abrir im�genes de 16 bits por p�xel
	cv::Mat pruebabien;
	cv::undistort(prueba, pruebabien, matrizcam, distcoef);
	cv::imwrite("pruebaRGBbien.JPG", pruebabien);


	// Correcci�n de la distorsi�n en im�genes del set de calibrado
	num_img = 0;
	for (directory_entry p : recursive_directory_iterator(ruta))  // iteraci�n en la carpeta
	{
		path ruta_archivo = p; // Cada archivo o subcarpeta localizado se utiliza como clase path
		string direc = ruta_archivo.string(); // convertir la ruta de clase path a clase string
		string nombre = ruta_archivo.filename().string(); // convertir el nombre del archivo de clase path a string

		if (nombre.size() > 8) // Si el nombre es grande de 8 caracteres significa que es una foto (evita carpetas).
		{
			if (nombre.compare(nombre.size() - 7, 7, "RGB.JPG") == 0)
			{
				num_img += 1;
				cv::Mat corregir = cv::imread(direc, CV_LOAD_IMAGE_COLOR);
				cv::Mat corregida;
				cv::undistort(corregir, corregida, matrizcam, distcoef);
				std::string nombre2 = "set_calibrado_corregido/";
				nombre2.append(to_string(num_img));
				nombre2.append("RGB.JPG");
				cv::imwrite(nombre2, corregida);
			}
		}
	}
	*/
	/// **********************************************************************************************************************************************************************
	/// **********************************************************************************************************************************************************************
}

void CalibraMono(string ruta_carpeta_entrada, string& banda_extension, int& num_k, string ruta_salida_deteccionesquina)
{
	/// CALIBRACI�N DE UN SET DE IM�GENES PARA UNA BANDA MONOCROM�TICA
	/// **********************************************************************************************************************************************************************
	// Cuidar de poner ruta de salida para las im�genes de detecci�n de esquinas y  (si se quieren sacar) y la
	path ruta(ruta_carpeta_entrada); //"D:/calibracion/"
	int num_img = 0;
	vector<vector<cv::Point2f>> coord_imagen; // Vector de vectores de puntos detectados en las im�genes
	vector<vector<cv::Point3f>> coord_obj; // Vector de vectores de puntos en coordenadas locales (se repiten siempre en cada imagen)
	cv::Mat matrizcam = Mat(Size(3, 3), CV_64F); // Matriz intr�nseca de la c�mara
	cv::Mat distcoef = Mat(Size(8, 1), CV_64F); // Matriz de coeficientes de distorsi�n de la c�mara
	vector<cv::Mat> rotmat; // Matriz extr�nseca de rotaciones de la c�mara
	vector<cv::Mat> trasmat; // Matriz extr�nseca de traslaciones de la c�mara

	for (directory_entry p : recursive_directory_iterator(ruta)) // iteraci�n en carpetas y subcarpetas de la ruta
	{
		path ruta_archivo = p; // Cada archivo o subcarpeta localizado se utiliza como clase path
		string direc = ruta_archivo.string(); // convertir la ruta de clase path a clase string
		string nombre = ruta_archivo.filename().string(); // convertir el nombre del archivo de clase path a string

		if (nombre.size() > 8) // Si el nombre es grande de 8 caracteres significa que es una foto (evita carpetas).
		{
			if (nombre.compare(nombre.size() - 7, 7, banda_extension) == 0) // Encuentra imagen banda buscada
			{
				num_img += 1; // cada vez que entra significa que encuentra una imagen de la banda que quiere

				Mat imagen = cv::imread(direc, CV_LOAD_IMAGE_UNCHANGED); // Para abrir im�genes de 16 bits por p�xel CV_LOAD_IMAGE_ANYDEPTH. Para abrir RGB -> CV_LOAD_IMAGE_COLOR
				imagen /= 255; // Para calibrar se necesita convertir a 8 bits por canal
				imagen.convertTo(imagen, CV_8UC1); // Le pongo un canal para el gris.
				cv::Size tamano(12, 8); // n�mero de esquinas a localizar
				vector<cv::Point2f> esquinas; // coordenadas de las esquinas detectadas en la imagen

				bool loc = cv::findChessboardCorners(imagen, tamano, esquinas, CV_CALIB_CB_ADAPTIVE_THRESH);
				if (loc == true) // si se localizan bien todas las esquinas es cuando se introduce dentro del set de calibraci�n
				{
					vector<cv::Point3f> obj; // vector de esquinas para la imagen de estudio en coordenadas locales
					for (int j = 1; j <= 8; j++)
					{
						for (int i = 1; i <= 12; i++)
						{
							obj.push_back({ (float(i) - 1.0f) * 30.0f,(float(j) - 1.0f) * 30.0f,0.0f }); // Tablero 30 x 30
						}
					}
					coord_obj.push_back(obj); // introduces las esquinas en coordenadas locales del tablero en el vector general
					coord_imagen.push_back(esquinas); // introduces las esquinas detectadas en coordenadas de la imagen en el vector general


					cv::cornerSubPix(imagen, esquinas, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, 0.1)); // funci�n para la detecci�n precisa de las esquinas
					//cv::cvtColor(imagen, imagen, CV_GRAY2RGB); // Se transforma la imagen a 3 canales de color para ponerle la detecci�n de esquinas en color.

					//cv::drawChessboardCorners(imagen, tamano, esquinas, loc); // Dibujo de las esquinas detectadas en colores

					//cv::imwrite(ruta_salida_deteccionesquina + std::to_string(num_img) + banda_extension, imagen);
				}

			}
		}

	}

	/// POSBILES CALIBRACIONES SEG�N EL N�MERO DE PAR�METROS DE DISTORSI�N RADIAL QUE SE QUIERAN UTILIZAR
	double error_repro;
	if (num_k == 3)
	{
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(1280, 960), matrizcam, distcoef, rotmat, trasmat); // Calibraci�n de la c�mara
	}
	else if (num_k == 2) // cv::TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 1000, 0.01)
	{
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(1280, 960), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_FIX_K3); // Calibraci�n de la c�mara
	}
	else if (num_k == 4)
	{
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(1280, 960), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_RATIONAL_MODEL | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6); // Calibraci�n de la c�mara
	}
	else if (num_k == 5)
	{
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(1280, 960), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_RATIONAL_MODEL | CV_CALIB_FIX_K6); // Calibraci�n de la c�mara
	}
	else if (num_k == 6)
	{
		error_repro = cv::calibrateCamera(coord_obj, coord_imagen, Size(1280, 960), matrizcam, distcoef, rotmat, trasmat, CV_CALIB_RATIONAL_MODEL); // Calibraci�n de la c�mara
	}

	double ancho = 4.8; // par�metros obtenidos mediante la multiplicaci�n del tama�o del p�xel (3.75 micras para monocrom�ticas) y de la resoluci�n de la imagen 
	double alto = 3.6; // especifican el ancho y el alto del tama�o del sensor. Probablemente por el tipo de obturador CCD mono y CMOS RGB.
	double fov_x;
	double fov_y;
	double dist_focal;
	cv::Point2d punto_prin;
	double ratio_aspecto;

	cv::calibrationMatrixValues(matrizcam, Size(1280, 960), ancho, alto, fov_x, fov_y, dist_focal, punto_prin, ratio_aspecto); // Par�metros f�sicos de la c�mara

	/// OBTENCI�N DE LAS MATRICES DE CALIBRACI�N
	vector<double>matrizfisica; // Vector para obtener la salida de par�metros
	matrizfisica.push_back(dist_focal);
	matrizfisica.push_back(ratio_aspecto);
	matrizfisica.push_back(punto_prin.x);
	matrizfisica.push_back(punto_prin.y);
	matrizfisica.push_back(fov_x);
	matrizfisica.push_back(fov_y);
	matrizfisica.push_back(error_repro);

	
	string nombre_salida = banda_extension;
	for (int i = 0; i <= 3; i++) // quitarle el .TIF a las bandas
	{
		nombre_salida.pop_back();
	}
	nombre_salida.append("_k");
	nombre_salida.append(to_string(num_k));
	nombre_salida.append(".yml");
	
	string salida = "Matriz_Fisica_";
	salida.append(nombre_salida);
	GuardarMatFisica(matrizfisica, salida);

	salida = "Matriz_Camara_";
	salida.append(nombre_salida);
	GuardarMatCamara(matrizcam, salida);

	salida = "Matriz_Distorsion_";
	salida.append(nombre_salida);
	GuardarMatDistorsion(distcoef, salida);

	/*
	// Correcci�n de la distorsi�n en im�genes de prueba
	cv::Mat prueba = cv::imread("pruebaV.TIF", CV_LOAD_IMAGE_ANYDEPTH); // Para abrir im�genes de 16 bits por p�xel
	cv::Mat pruebabien;
	cv::undistort(prueba, pruebabien, matrizcam, distcoef);
	cv::imwrite("pruebaVbien.TIF", pruebabien);
	*/

	/*
	// Correcci�n de la distorsi�n en im�genes del set de calibrado
	num_img = 0;
	for (directory_entry p : recursive_directory_iterator(ruta))  // iteraci�n en la carpeta
	{
		path ruta_archivo = p; // Cada archivo o subcarpeta localizado se utiliza como clase path
		string direc = ruta_archivo.string(); // convertir la ruta de clase path a clase string
		string nombre = ruta_archivo.filename().string(); // convertir el nombre del archivo de clase path a string

		if (nombre.size() > 8) // Si el nombre es grande de 8 caracteres significa que es una foto (evita carpetas).
		{
			if (nombre.compare(nombre.size() - 7, 7, banda) == 0)
			{
				num_img += 1;
				cv::Mat corregir = cv::imread(direc, CV_LOAD_IMAGE_ANYDEPTH);
				corregir /= 255;
				corregir.convertTo(corregir, CV_8UC1);
				cv::Mat corregida;
				cv::undistort(corregir, corregida, matrizcam, distcoef);
				std::string nombre2 = "set_calibrado_corregido/";
				nombre2.append(to_string(num_img));
				nombre2.append(banda);
				cv::imwrite(nombre2, corregida);
			}
		}
	}
	*/

	/// **********************************************************************************************************************************************************************
	/// **********************************************************************************************************************************************************************
}

void CorrigeImagenes(Mat& mat_cam, Mat& dist_coef, string& banda, string ruta_img_entrada, string ruta_img_salida)
{
	path ruta(ruta_img_entrada); // "D:/calibracion/"
	int num_img = 0;

	for (directory_entry p : recursive_directory_iterator(ruta))  // iteraci�n en la carpeta
	{
		path ruta_archivo = p; // Cada archivo o subcarpeta localizado se utiliza como clase path
		string direc = ruta_archivo.string(); // convertir la ruta de clase path a clase string
		string nombre = ruta_archivo.filename().string(); // convertir el nombre del archivo de clase path a string

		if (nombre.size() > 8) // Si el nombre es grande de 8 caracteres significa que es una foto (evita carpetas).
		{
			if (nombre.compare(nombre.size() - 7, 7, banda) == 0)
			{
				num_img += 1;
				cv::Mat corregir = cv::imread(direc, CV_LOAD_IMAGE_ANYDEPTH); // En el caso de TIF con 10 bits de profundidad
				corregir /= 255;
				corregir.convertTo(corregir, CV_8UC1);
				cv::Mat corregida;
				cv::undistort(corregir, corregida, mat_cam, dist_coef);
				std::string nombre2 = ruta_img_salida;
				string nombre = banda;
				for (int i = 0; i <= 3; i++)
				{
					nombre.pop_back();
				}
				int num_k = dist_coef.size().width-2;
				nombre2.append(to_string(num_img) + nombre + "_k" + to_string(num_k) + ".TIF");

				cv::imwrite(nombre2, corregida);
			}
		}
	}
}

void CorrigeImagenesRGB(Mat& mat_cam, Mat& dist_coef, string& banda, string ruta_img_entrada, string ruta_img_salida)
{
	path ruta(ruta_img_entrada); // "D:/calibracion/"
	int num_img = 0;

	for (directory_entry p : recursive_directory_iterator(ruta))  // iteraci�n en la carpeta
	{
		path ruta_archivo = p; // Cada archivo o subcarpeta localizado se utiliza como clase path
		string direc = ruta_archivo.string(); // convertir la ruta de clase path a clase string
		string nombre = ruta_archivo.filename().string(); // convertir el nombre del archivo de clase path a string

		if (nombre.size() > 8) // Si el nombre es grande de 8 caracteres significa que es una foto (evita carpetas).
		{
			if (nombre.compare(nombre.size() - 7, 7, banda) == 0)
			{
				num_img += 1;
				cv::Mat corregir = cv::imread(direc, CV_LOAD_IMAGE_COLOR); // En el caso de JPG son 8 bits de profundidad en RGB.
				cv::Mat corregida;
				cv::undistort(corregir, corregida, mat_cam, dist_coef);
				std::string nombre2 = ruta_img_salida;
				string nombre = banda;
				for (int i = 0; i <= 3; i++)
				{
					nombre.pop_back();
				}
				int num_k = dist_coef.size().width - 2;
				nombre2.append(to_string(num_img) + nombre + "_k" + to_string(num_k) + ".JPG");

				cv::imwrite(nombre2, corregida);
			}
		}
	}
}
