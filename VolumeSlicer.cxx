// =========================================================================
// Base example to interactively cut a volume with a plane
// =========================================================================

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <vtkColorTransferFunction.h>
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

#include <vtkPlaneWidget.h>
#include <vtkClipVolume.h>
#include <vtkPlane.h>
#include <vtkClipPolyData.h>

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
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

  // Prepare ray-caster
  vtkNew< vtkFixedPointVolumeRayCastMapper > volume_mapper;
  volume_mapper->SetInputConnection( volume_reader->GetOutputPort( ) );

  // Volume rendering properties
  volume_properties->SetColor( volume_color );
  volume_properties->SetScalarOpacity( volume_opacity );
  volume_properties->SetGradientOpacity( volume_gradient );
  volume_properties->SetInterpolationTypeToLinear( );

  // Raycast actor
  vtkNew< vtkVolume > volume;
  volume->SetMapper( volume_mapper );
  volume->SetProperty( volume_properties );

  // ========== CAJA ==========
  vtkNew< vtkNamedColors > colors;

  vtkNew< vtkOutlineFilter > outline;
  outline->SetInputConnection( volume_reader->GetOutputPort() );

  vtkNew< vtkPolyDataMapper > mapper;
  mapper->SetInputConnection( outline->GetOutputPort() );

  vtkNew< vtkActor > actor;
  actor->SetMapper( mapper );
  actor->GetProperty( )->SetColor( colors->GetColor3d( "Red" ).GetData() );

  // Prepare rendering objects
  vtkNew< vtkInteractorStyleSwitch > style;
  vtkNew< vtkRenderer > ren;
  vtkNew< vtkRenderWindow > win;
  vtkNew< vtkRenderWindowInteractor > iren;

  volume_reader->Update();

  style->SetCurrentStyleToTrackballCamera( );
  style->SetDefaultRenderer( ren );
  ren->SetBackground( 0.7, 0.7, 0.7 );
  win->AddRenderer( ren );
  iren->SetRenderWindow( win );
  iren->SetInteractorStyle( style );

  // ========== PLANE WIDGET ==========
  // plane mueve el corte del raycasting en vivo
  // clip_plane solo se actualiza al soltar el mouse para no recalcular vtkClipVolume todo el tiempo
  vtkNew< vtkPlane > plane;
  vtkNew< vtkPlane > clip_plane;

  vtkNew< vtkPlaneWidget > plane_widget;
  plane_widget->SetInteractor( iren );
  plane_widget->SetInputConnection( volume_reader->GetOutputPort() );
  plane_widget->PlaceWidget();
  plane_widget->GetPlane( plane );
  plane_widget->GetPlane( clip_plane );

  plane_widget->On();

  auto update_plane = []( vtkObject* caller, unsigned long, void* client_data, void* )
  {
    auto* widget = static_cast< vtkPlaneWidget* >( caller );
    auto* p = static_cast< vtkPlane* >( client_data );
    widget->GetPlane( p );
  };

  vtkNew< vtkCallbackCommand > callback;
  callback->SetCallback( update_plane );
  callback->SetClientData( plane.GetPointer( ) );
  plane_widget->AddObserver( vtkCommand::InteractionEvent, callback );

  vtkNew< vtkCallbackCommand > end_callback;
  end_callback->SetCallback( update_plane );
  end_callback->SetClientData( clip_plane.GetPointer( ) );
  plane_widget->AddObserver( vtkCommand::EndInteractionEvent, end_callback );

  // ========== EJES DE ORIENTACION ==========
  vtkNew< vtkAxesActor > axes_actor;
  vtkNew< vtkOrientationMarkerWidget > axes_widget;
  axes_widget->SetOrientationMarker( axes_actor );
  axes_widget->SetInteractor( iren );
  axes_widget->SetViewport( 0.0, 0.0, 0.2, 0.2 );
  axes_widget->SetEnabled( 1 );
  axes_widget->InteractiveOff( );

  // ========== CLIP VOLUME ==========
  // raycasting del volumen original, cortado por el plano
  volume_mapper->AddClippingPlane( plane );

  // volumen cortado con vtkClipVolume
  vtkNew< vtkClipVolume > clip;
  clip->SetInputConnection( volume_reader->GetOutputPort() );
  clip->SetClipFunction( clip_plane );
  clip->SetValue( 0.0 );
  clip->InsideOutOff( );
  clip->GenerateClippedOutputOff( );

  vtkNew< vtkDataSetMapper > clipped_mapper;
  clipped_mapper->SetInputConnection( clip->GetOutputPort() );
  clipped_mapper->SetLookupTable( volume_color );
  clipped_mapper->SetColorModeToMapScalars( );
  clipped_mapper->SetScalarRange( volume_reader->GetOutput( )->GetScalarRange( ) );
  clipped_mapper->ScalarVisibilityOn();

  vtkNew< vtkActor > clipped_actor;
  clipped_actor->SetMapper( clipped_mapper );

  // ========== SUPERFICIE ==========
  double iso_value = 300.0;

  // malla con marching cubes, tambien cortada por el plano
  vtkNew< vtkMarchingCubes > surface;
  surface->SetInputConnection( volume_reader->GetOutputPort( ) );
  surface->SetValue( 0, iso_value );
  surface->ComputeNormalsOn( );

  vtkNew< vtkClipPolyData > surface_clip;
  surface_clip->SetInputConnection( surface->GetOutputPort( ) );
  surface_clip->SetClipFunction( clip_plane );
  surface_clip->SetValue( 0.0 );
  surface_clip->InsideOutOff( );

  vtkNew< vtkPolyDataMapper > surface_mapper;
  surface_mapper->SetInputConnection( surface_clip->GetOutputPort() );
  surface_mapper->ScalarVisibilityOff( );

  vtkNew< vtkActor > surface_actor;
  surface_actor->SetMapper( surface_mapper );
  surface_actor->GetProperty( )->SetColor( colors->GetColor3d( "Ivory" ).GetData() );

  // ========== TECLADO ==========
  /*
  v - volumen (raycasting)
  k - volumen cortado (vtkClipVolume)
  m - malla (marching cubes)
  x - invierte el lado del corte
  */
  struct ViewState
  {
    vtkVolume* volume;
    vtkActor* clipped_actor;
    vtkActor* surface_actor;
    vtkPlaneWidget* widget;
    vtkPlane* plane;
    vtkPlane* clip_plane;
  };
  ViewState state;
  state.volume = volume;
  state.clipped_actor = clipped_actor;
  state.surface_actor = surface_actor;
  state.widget = plane_widget;
  state.plane = plane;
  state.clip_plane = clip_plane;

  auto key_press = []( vtkObject* caller, unsigned long, void* client_data, void* )
  {
    auto* i = static_cast< vtkRenderWindowInteractor* >( caller );
    auto* s = static_cast< ViewState* >( client_data );
    std::string key = i->GetKeySym( );
    if( key == "v" || key == "k" || key == "m" )
    {
      s->volume->SetVisibility( key == "v" );
      s->clipped_actor->SetVisibility( key == "k" );
      s->surface_actor->SetVisibility( key == "m" );
    }
    else if( key == "x" )
    {
      double n[ 3 ];
      s->widget->GetNormal( n );
      n[ 0 ] = -n[ 0 ];
      n[ 1 ] = -n[ 1 ];
      n[ 2 ] = -n[ 2 ];
      s->widget->SetNormal( n );
      s->widget->GetPlane( s->plane );
      s->widget->GetPlane( s->clip_plane );
    } // end if
    i->GetRenderWindow( )->Render( );
  };

  vtkNew< vtkCallbackCommand > key_callback;
  key_callback->SetCallback( key_press );
  key_callback->SetClientData( &state );
  iren->AddObserver( vtkCommand::KeyPressEvent, key_callback );

  // Add actors
  ren->AddViewProp( volume );
  ren->AddViewProp( actor );
  ren->AddActor( clipped_actor );
  ren->AddActor( surface_actor );

  // vista inicial (raycasting)
  clipped_actor->SetVisibility( 0 );
  surface_actor->SetVisibility( 0 );

  ren->ResetCamera( );
  ren->ResetCameraClippingRange( );

  // Finish setup and start main loop
  win->SetSize( 800, 800 );
  win->SetWindowName( "Volume slicer" );
  win->Render( );
  iren->Start( );

  return( EXIT_SUCCESS );
}

// eof - VolumeSlicer.cxx
