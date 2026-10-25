// =========================================================================
// Base example to interactively cut a volume with a plane
// =========================================================================

#include <fstream>
#include <iostream>
#include <string>

#include <vtkColorTransferFunction.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkMetaImageReader.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>

#include <vtkPlaneWidget.h>
#include <vtkPlane.h>
#include <vtkClipVolume.h>
#include <vtkDataSetMapper.h>

int main( int argc, char** argv )
{
  if( argc < 3 )
  {
    std::cout
      << "Usage: " << argv[ 0 ]
      << " file.[mhd|mha] volume_properties.txt"
      << std::endl;
    return( EXIT_FAILURE );
  } // end if
  std::string input_filename = argv[ 1 ];
  std::string vp_filename = argv[ 2 ];

  // Read properties file
  std::ifstream vp_file( vp_filename, std::ios::binary | std::ios::ate );
  if( !vp_file )
  {
    std::cerr << "Could not open file: " << vp_filename << std::endl;
    return( EXIT_FAILURE );
  } // end if
  std::streamsize vp_file_size = vp_file.tellg( );
  vp_file.seekg( 0, std::ios::beg );
  std::string vp( static_cast< std::size_t >( vp_file_size ), '\0' );
  if( !vp_file.read( vp.data( ), vp_file_size ) )
  {
    std::cerr << "Could not read file: " << vp_filename << std::endl;
    return( EXIT_FAILURE );
  } // end if

  vtkNew< vtkNamedColors > colors;

  // Create volume raycasting properties
  vtkNew< vtkColorTransferFunction > volume_color;
  vtkNew< vtkPiecewiseFunction > volume_opacity;
  vtkNew< vtkPiecewiseFunction > volume_gradient;
  vtkNew< vtkVolumeProperty > volume_properties;
  std::istringstream vp_str( vp );
  std::string line;

  auto trim = []( const std::string& s ) -> std::string
    {
      const char* ws = " \t";
      std::size_t start = s.find_first_not_of( ws );
      if( start == std::string::npos )
        return( "" );
      std::size_t end = s.find_last_not_of( ws );
      return( s.substr( start, end - start + 1 ) );
    };


  while( std::getline( vp_str, line ) )
  {
    std::string l = trim( line );
    if( l != "" )
    {
      std::size_t p = l.find_first_of( " \t" );
      if( p != std::string::npos )
      {
        std::string cmd = l.substr( 0, p );
        std::istringstream values( l.substr( p ) );
        if( cmd == "color" )
        {
          double i, r, g, b;
          values >> i >> r >> g >> b;
          volume_color->AddRGBPoint( i, r, g, b );
        }
        else if( cmd == "opacity" )
        {
          double i, v;
          values >> i >> v;
          volume_opacity->AddPoint( i, v );
        }
        else if( cmd == "gradient" )
        {
          double i, v;
          values >> i >> v;
          volume_gradient->AddPoint( i, v );
        }
        else if( cmd == "shade" )
        {
          std::string v;
          values >> v;
          if( v == "on" )
            volume_properties->ShadeOn( );
          else if( v == "off" )
            volume_properties->ShadeOff( );
        }
        else if( cmd == "ambient" )
        {
          double v;
          values >> v;
          volume_properties->SetAmbient( v );
        }
        else if( cmd == "diffuse" )
        {
          double v;
          values >> v;
          volume_properties->SetDiffuse( v );
        }
        else if( cmd == "specular" )
        {
          double v;
          values >> v;
          volume_properties->SetSpecular( v );
        } // end if
      } // end if
    } // end if
  } // end while

  // Read a 3D image (volume)
  vtkNew< vtkMetaImageReader > volume_reader;
  volume_reader->SetFileName( argv[ 1 ] );

  // caja envolvente
  /**
   * Calcula los limites del volumen y crea a su alrededor las aristas 
   * envolventes. Input -> El volumen cargado, Output -> La caja envolvente.
   */
  vtkNew< vtkOutlineFilter > outline;
  outline->SetInputConnection( volume_reader->GetOutputPort( ) );

  // mapper
  /**
   * Es el mapper de las mallas, convierte el vtkPolyData en buffers y 
   * ordenes de dibujo de OpenGL. Input -> La caja envolvente, Output -> Los buffers de OpenGL.
   */
  vtkNew< vtkPolyDataMapper > mapper;
  mapper->SetInputConnection( outline->GetOutputPort( ) );

  // actor
  /**
   * Se crea el actor de la caja envolvente, se le asigna el mapper y se configura su color.
   */
  vtkNew< vtkActor > boxActor;
  boxActor->SetMapper( mapper );
  boxActor->GetProperty( )->SetColor( colors->GetColor3d( "Red" ).GetData() );
  //actor->GetProperty( )->SetLineWidth( 0.7 );

  // Prepare ray-caster
  /** ¿Que se esta haciendo?
   * Se define el mapper del volumen y se le asigna el volumen cargado.
   */
  vtkNew< vtkFixedPointVolumeRayCastMapper > volume_mapper;
  volume_mapper->SetInputConnection( volume_reader->GetOutputPort( ) );

  // Volume rendering properties
  /**
   * Configura las propiedades de renderizado del volumen, con las variables
   * cargadas en memoria desde el archivo de propiedades. 
   */
  volume_properties->SetColor( volume_color );
  volume_properties->SetScalarOpacity( volume_opacity );
  volume_properties->SetGradientOpacity( volume_gradient );
  volume_properties->SetInterpolationTypeToLinear( );

  // Raycast actor
  /**
   * Se crea el actor del volumen, se crea con el parametro volume_mapper
   * (volumen cargado desde el archivo) y se le asignan las propiedades el volumen.
   */
  vtkNew< vtkVolume > volume;
  volume->SetMapper( volume_mapper ); //SetMapper le dice como dibujarse
  volume->SetProperty( volume_properties ); //Con que apariencia dibujarse


  // Prepare rendering objects
  vtkNew< vtkInteractorStyleSwitch > style; // Camarografo que sigue las instrucciones del Staff
  vtkNew< vtkRenderer > ren; // Creación del escenario con su camara y focos
  vtkNew< vtkRenderWindow > win; // El teatro donde se dibuja el escenario
  vtkNew< vtkRenderWindowInteractor > iren; // Staff que escucha al publico (eventos de teclado y mouse)

  // plane
  /**
   * Se lee antes de realizar el render, pues necesitamos conocer los limites
   * del volumen para poder dibujar la caja envolvente.
   */
  volume_reader->Update();

  //=========================================================================================
  // Desarrollo del Taller 
  //=========================================================================================
  


  //========== VTK PLANE WIDGET ==========


  vtkNew< vtkPlane > planeActor;

  vtkNew< vtkPlaneWidget > plane_widget;
  plane_widget->SetInteractor( iren );

  plane_widget->On();


  // ======== IMPLEMENTACION AXES TRANSFORM WIDGET ==========
  vtkNew< vtkAxesActor > axesActor;
  vtkNew< vtkOrientationMarkerWidget > axesWidget;
  axesWidget->SetOutlineColor( 0.93, 0.57, 0.13 );
  axesWidget->SetOrientationMarker( axesActor );
  axesWidget->SetInteractor( iren );
  axesWidget->SetViewport( 0.0, 0.0, 0.2, 0.2 );
  axesWidget->SetEnabled( 1 );
  axesWidget->InteractiveOn( );

  // Interaccion con el mouse y el teclado
  style->SetCurrentStyleToTrackballCamera( );

  /** ===========================================
   * Configuración de la escena:
   *  ===========================================
   */
  ren->SetBackground( 0.7, 0.7, 0.7 );
  win->AddRenderer( ren ); // añade el escenario al teatro
  iren->SetRenderWindow( win ); // le dice al staff que teatro debe escuchar
  iren->SetInteractorStyle( style ); // asigna al staff el camarografo que debe seguir


  // Volume clipper
  vtkNew< vtkClipVolume > clipper;
  clipper->SetInputConnection(volume_reader->GetOutputPort());
  clipper->SetClipFunction(plane);

  vtkNew< vtkDataSetMapper > clipMapper;
  clipMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew< vtkActor > clipActor;
  clipActor->SetMapper(clipMapper);


  // ====== AÑADIR ACTORES A LA ESCENA ======
  ren->AddViewProp( volume );
  ren->AddViewProp( boxActor );
  ren->AddViewProp( axesActor );

  ren->AddActor( clipActor );

  // Finish setup and start main loop
  win->SetSize( 800, 800 );
  win->SetWindowName( "Volume slicer Taller 3 UwU" );
  win->Render( );

  //Sin iren->Start(), no hay nada que mantenga vivo el programa, 
  //por lo que se cierra inmediatamente despues de renderizar la ventana.
  iren->Start( );

  return( EXIT_SUCCESS );
}

// eof - VolumeSlicer.cxx
