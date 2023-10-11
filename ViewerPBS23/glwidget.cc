// Skeleton Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#include <glwidget.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "./mesh_io.h"
#include "./triangle_mesh.h"

namespace {

const double kFieldOfView = 60;
const double kZNear = 0.0001;
const double kZFar = 100;

const float skybox_scale = 5.0f;

float skyboxVertices[24] =
{
    //   Coordinates
    -1.0f * skybox_scale, -1.0f * skybox_scale, -1.0f * skybox_scale,	//        4--------5
     1.0f * skybox_scale, -1.0f * skybox_scale, -1.0f * skybox_scale,	//       /|       /|
    -1.0f * skybox_scale, -1.0f * skybox_scale,  1.0f * skybox_scale,   //      6--------7 |
     1.0f * skybox_scale, -1.0f * skybox_scale,  1.0f * skybox_scale,	//      | |      | |
    -1.0f * skybox_scale,  1.0f * skybox_scale, -1.0f * skybox_scale,	//      | 0------|-1
     1.0f * skybox_scale,  1.0f * skybox_scale, -1.0f * skybox_scale,	//      |/       |/
    -1.0f * skybox_scale,  1.0f * skybox_scale,  1.0f * skybox_scale,   //      2--------3
     1.0f * skybox_scale,  1.0f * skybox_scale,  1.0f * skybox_scale
};

unsigned int skyboxIndices[36] =
{
    // Top
    4, 7, 6,
    4, 5, 7,
    // Bottom
    0, 3, 1,
    0, 2, 3,
    // Front
    0, 1, 4,
    4, 1, 5,
    // Back
    6, 3, 2,
    6, 7, 3,
    // Left
    6, 2, 0,
    4, 6, 0,
    // Right
    1, 3, 7,
    7, 5, 1
};


const std::vector<std::vector<std::string>> kShaderFiles = {
                {"../shaders/phong.vert",                   "../shaders/phong.frag"},
                {"../shaders/texMap.vert",                  "../shaders/texMap.frag"},
                {"../shaders/reflection.vert",              "../shaders/reflection.frag"},
                {"../shaders/pbs.vert",                     "../shaders/pbs.frag"},
                {"../shaders/ibl-pbs.vert",                 "../shaders/ibl-pbs.frag"},
                {"../shaders/sky.vert",                     "../shaders/sky.frag"}};//sky needs to be the last one

const int kVertexAttributeIdx = 0;
const int kNormalAttributeIdx = 1;
const int kTexCoordAttributeIdx = 2;


bool ReadFile(const std::string filename, std::string *shader_source) {
  std::ifstream infile(filename.c_str());

  if (!infile.is_open() || !infile.good()) {
    std::cerr << "Error " + filename + " not found." << std::endl;
    return false;
  }

  std::stringstream stream;
  stream << infile.rdbuf();
  infile.close();

  *shader_source = stream.str();
  return true;
}

bool LoadImage(const std::string &path, GLuint cube_map_pos) {
  QImage image;
  bool res = image.load(path.c_str());
  if (res) {    
    QImage gl_image = image.mirrored();
    glTexImage2D(cube_map_pos, 0, GL_RGBA, image.width(), image.height(), 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  return res;
}

bool LoadCubeMap(const QString &dir) {
  std::string path = dir.toUtf8().constData();
  bool res =   LoadImage(path + "/right", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
  res = res && LoadImage(path + "/left", GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
  res = res && LoadImage(path + "/top", GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
  res = res && LoadImage(path + "/bottom", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
  res = res && LoadImage(path + "/back", GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
  res = res && LoadImage(path + "/front", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

  if (res) {
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // This might help with seams on some systems
    //glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  }

  return res;
}

bool LoadProgram(const std::string &vertex, const std::string &fragment,
                 QOpenGLShaderProgram *program) {
  std::string vertex_shader, fragment_shader;
  bool res =
      ReadFile(vertex, &vertex_shader) && ReadFile(fragment, &fragment_shader);

  if (res) {
    program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                     vertex_shader.c_str());
    program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                     fragment_shader.c_str());
    program->bindAttributeLocation("vertex", kVertexAttributeIdx);
    program->bindAttributeLocation("normal", kNormalAttributeIdx);
    program->bindAttributeLocation("texCoord", kTexCoordAttributeIdx);
    program->link();
  }

  return res;
}

}  // namespace

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent),
      initialized_(false),
      width_(0.0),
      height_(0.0),
      currentShader_(0),
      currentTexture_(0),
      fresnel_(0.2, 0.2, 0.2),
      skyVisible_(true),
      metalness_(0),
      roughness_(0){
  setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget() {
  if (initialized_) {
    glDeleteTextures(1, &specular_map_);
    glDeleteTextures(1, &diffuse_map_);
  }
}

bool GLWidget::LoadModel(const QString &filename) {
  std::string file = filename.toUtf8().constData();
  size_t pos = file.find_last_of(".");
  std::string type = file.substr(pos + 1);

  std::unique_ptr<data_representation::TriangleMesh> mesh =
      std::make_unique<data_representation::TriangleMesh>();

  bool res = false;
  /*if (type.compare("ply") == 0) {
    res = data_representation::ReadFromPly(file, mesh.get());
  } else if (type.compare("obj") == 0) {
    res = data_representation::ReadFromObj(file, mesh.get());
  } else if(type.compare("null") == 0) {
    res = data_representation::CreateSphere(mesh.get());
  }*/
      res = data_representation::CreateSphere(mesh.get());

  if (res) {
    mesh_.reset(mesh.release());
    camera_.UpdateModel(mesh_->min_, mesh_->max_);
    //mesh_->computeNormals();

    // Generate VAO and Buffers
    glGenVertexArrays(1, &VAO);

    glGenBuffers(1, &VBO_v);
    glGenBuffers(1, &VBO_n);
    glGenBuffers(1, &VBO_tc);
    glGenBuffers(1, &VBO_i);

    // Bind VAO
    glBindVertexArray(VAO);

    // Configure coordinate VBO -> attrib location 0
    glBindBuffer(GL_ARRAY_BUFFER, VBO_v);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*mesh_->vertices_.size(), &mesh_->vertices_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Configure normal VBO -> attrib location 1
    glBindBuffer(GL_ARRAY_BUFFER, VBO_n);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*mesh_->normals_.size(), &mesh_->normals_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // Configure texCoords VBO -> attrib location 2
    glBindBuffer(GL_ARRAY_BUFFER, VBO_tc);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*mesh_->texCoords_.size(), &mesh_->texCoords_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    // Configure coordinate EBO -> elements
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_i);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*mesh_->faces_.size(), &mesh_->faces_[0], GL_STATIC_DRAW);
    
    // Unbind VAO
    glBindVertexArray(0);

    
    //SKY BOX
    // --------------------------------------------------

    // Store skyVertices_ defined by my skyboxVertices
    for(unsigned int i=0; i<sizeof(skyboxVertices); i++)
    {
        skyVertices_.push_back(skyboxVertices[i]);
    }

    // Store skyFaces_ defined by my skyboxIndices
    for(unsigned int i=0; i<sizeof(skyboxIndices); i++)
    {
        skyFaces_.push_back(skyboxIndices[i]);
    }

    // Generate Buffers
    glGenVertexArrays(1, &VAO_sky);
    glGenBuffers(1, &VBO_v_sky);
    glGenBuffers(1, &VBO_i_sky);

    // Bind Buffers
    glBindVertexArray(VAO_sky);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_v_sky);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * skyVertices_.size(), &(skyVertices_[0]), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_i_sky);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * skyFaces_.size(), &(skyFaces_[0]), GL_STATIC_DRAW);

    // Unbind Buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    emit SetFaces(QString(std::to_string(mesh_->faces_.size() / 3).c_str()));
    emit SetVertices(
        QString(std::to_string(mesh_->vertices_.size() / 3).c_str()));
    return true;
  }

  return false;
}

bool GLWidget::LoadSpecularMap(const QString &dir) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
  bool res = LoadCubeMap(dir);
  // Load the cubemap texture and THEN generate mipmaps and set min_filter parameter
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  return res;
}

bool GLWidget::LoadDiffuseMap(const QString &dir) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse_map_);
  bool res = LoadCubeMap(dir);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  return res;
}

bool GLWidget::LoadColorMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, color_map_);
    bool res = LoadImage(filename.toStdString(), GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return res;

}

bool GLWidget::LoadRoughnessMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, roughness_map_);
    bool res = LoadImage(filename.toStdString(), GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return res;
}

bool GLWidget::LoadMetalnessMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, metalness_map_);
    bool res = LoadImage(filename.toStdString(), GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return res;
}

bool GLWidget::LoadBRDFLUTMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, brdfLUT_map_);
    bool res = LoadImage(filename.toStdString(), GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return res;
}

void GLWidget::initializeGL() {
  glewInit();

  // Draw as wireframe
  //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  // Draw as usual
  //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

  //initializing opengl state
  glEnable(GL_NORMALIZE);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);

  //generating needed textures
  glGenTextures(1, &specular_map_);
  glGenTextures(1, &diffuse_map_);
  glGenTextures(1, &color_map_);
  glGenTextures(1, &roughness_map_);
  glGenTextures(1, &metalness_map_);
  glGenTextures(1, &brdfLUT_map_);

  //create shader programs
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//phong
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//texture mapping
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//reflection
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//simple pbs
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//ibl pbs
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//sky

  //load vertex and fragment shader files
  bool res =   LoadProgram(kShaderFiles[0][0],   kShaderFiles[0][1],    programs_[0].get());
  res = res && LoadProgram(kShaderFiles[1][0],   kShaderFiles[1][1],    programs_[1].get());
  res = res && LoadProgram(kShaderFiles[2][0],   kShaderFiles[2][1],    programs_[2].get());
  res = res && LoadProgram(kShaderFiles[3][0],   kShaderFiles[3][1],    programs_[3].get());
  res = res && LoadProgram(kShaderFiles[4][0],   kShaderFiles[4][1],    programs_[4].get());
  res = res && LoadProgram(kShaderFiles[5][0],   kShaderFiles[5][1],    programs_[5].get());

  if (!res) exit(0);

  LoadModel(".null");//create an sphere

  initialized_ = true;

  // Load the color map from the relative filepath.
  bool colorMapLoaded = LoadColorMap("../textures/antique-grate1-bl/antique-grate1-albedo.png");
  if (!colorMapLoaded) {
      qWarning() << "Failed to load color map texture";
  }
  // Load the roughness map from the relative filepath.
  bool roughnessMapLoaded = LoadRoughnessMap("../textures/antique-grate1-bl/antique-grate1-roughness.png");
  if (!roughnessMapLoaded) {
      qWarning() << "Failed to load roughness map texture";
  }
  // Load the metalness map from the relative filepath.
  bool metalnessMapLoaded = LoadMetalnessMap("../textures/antique-grate1-bl/antique-grate1-metallic.png");
  if (!metalnessMapLoaded) {
      qWarning() << "Failed to load metalness map texture";
  }
  // Load the brdfLUT map from the relative filepath.
  bool brdfLUTMapLoaded = LoadBRDFLUTMap("../textures/ibl/ibl_brdf_lut.png");
  if (!brdfLUTMapLoaded) {
      qWarning() << "Failed to load brdfLUT map texture";
  }

  // Load the specular cube map
  bool specularCubeMapLoaded = LoadSpecularMap("../textures/desert_specular");
  if(!specularCubeMapLoaded)
  {
      qWarning() << "Failed to load cube map texture";
  }

  // Load the diffuse cube map
  bool diffuseCubeMapLoaded = LoadDiffuseMap("../textures/desert_diffuse");
  if(!diffuseCubeMapLoaded)
  {
      qWarning() << "Failed to load cube map texture";
  }

  // Seamless skybox
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void GLWidget::resizeGL(int w, int h) {
  if (h == 0) h = 1;
  width_ = w;
  height_ = h;

  camera_.SetViewport(0, 0, w, h);
  camera_.SetProjection(kFieldOfView, kZNear, kZFar);
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    camera_.StartRotating(event->x(), event->y());
  }
  if (event->button() == Qt::RightButton) {
    camera_.StartZooming(event->x(), event->y());
  }
  updateGL();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
  camera_.SetRotationX(event->y());
  camera_.SetRotationY(event->x());
  camera_.SafeZoom(event->y());
  updateGL();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    camera_.StopRotating(event->x(), event->y());
  }
  if (event->button() == Qt::RightButton) {
    camera_.StopZooming(event->x(), event->y());
  }
  updateGL();
}

void GLWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Up) camera_.Zoom(-1);
  if (event->key() == Qt::Key_Down) camera_.Zoom(1);

  if (event->key() == Qt::Key_Left) camera_.Rotate(-1);
  if (event->key() == Qt::Key_Right) camera_.Rotate(1);

  if (event->key() == Qt::Key_W) camera_.Zoom(-1);
  if (event->key() == Qt::Key_S) camera_.Zoom(1);

  if (event->key() == Qt::Key_A) camera_.Rotate(-1);
  if (event->key() == Qt::Key_D) camera_.Rotate(1);

  if (event->key() == Qt::Key_R) {
      for(auto i = 0; i < programs_.size(); ++i) {
          programs_[i].reset();
          programs_[i] = std::make_unique<QOpenGLShaderProgram>();
          LoadProgram(kShaderFiles[i][0], kShaderFiles[i][1], programs_[i].get());
      }
  }

  updateGL();
}

void GLWidget::paintGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (initialized_) {
        camera_.SetViewport();

        // Set the MVP matrices
        Eigen::Matrix4f projection = camera_.SetProjection();
        Eigen::Matrix4f view = camera_.SetView();
        Eigen::Matrix4f model = camera_.SetModel();

        //compute normal matrix
        Eigen::Matrix4f t = view * model;
        Eigen::Matrix3f normal;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) normal(i, j) = t(i, j);
                normal = normal.inverse().transpose();

        if (mesh_ != nullptr) {
            GLint projection_location, view_location, model_location,
            normal_matrix_location, specular_map_location, diffuse_map_location, brdfLUT_map_location,
            fresnel_location, color_map_location, roughness_map_location, metalness_map_location,
            current_text_location, light_location, roughness_location, metalness_location, camera_location,
            albedo_location, emissivity_location;

            //MESH-----------------------------------------------------------------------------------------
            //general shader setting

            programs_[currentShader_]->bind();

            projection_location       = programs_[currentShader_]->uniformLocation("projection");
            view_location             = programs_[currentShader_]->uniformLocation("view");
            model_location            = programs_[currentShader_]->uniformLocation("model");
            normal_matrix_location    = programs_[currentShader_]->uniformLocation("normal_matrix");
            specular_map_location     = programs_[currentShader_]->uniformLocation("specular_map");
            diffuse_map_location      = programs_[currentShader_]->uniformLocation("diffuse_map");
            brdfLUT_map_location      = programs_[currentShader_]->uniformLocation("brdfLUT_map");
            color_map_location        = programs_[currentShader_]->uniformLocation("color_map");
            roughness_map_location    = programs_[currentShader_]->uniformLocation("roughness_map");
            metalness_map_location    = programs_[currentShader_]->uniformLocation("metalness_map");
            current_text_location     = programs_[currentShader_]->uniformLocation("current_texture");
            fresnel_location          = programs_[currentShader_]->uniformLocation("fresnel");
            light_location            = programs_[currentShader_]->uniformLocation("light");
            camera_location           = programs_[currentShader_]->uniformLocation("camPos");
            roughness_location        = programs_[currentShader_]->uniformLocation("roughness");
            metalness_location        = programs_[currentShader_]->uniformLocation("metalness");
            albedo_location           = programs_[currentShader_]->uniformLocation("albedo");
            emissivity_location       = programs_[currentShader_]->uniformLocation("emissivity");

            // MVP + normal_matrix
            glUniformMatrix4fv(projection_location, 1, GL_FALSE, projection.data());
            glUniformMatrix4fv(view_location, 1, GL_FALSE, view.data());
            glUniformMatrix4fv(model_location, 1, GL_FALSE, model.data());
            glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, normal.data());

            // Cubemap specular
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
            glUniform1i(specular_map_location, 0);
            // Cubemap diffuse
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse_map_);
            glUniform1i(diffuse_map_location, 1);

            // Texture_2D
            // Texture unit 0 color_map_
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, color_map_);
            glUniform1i(color_map_location, 0);
            // Texture unit 1 roughness_map_
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, roughness_map_);
            glUniform1i(roughness_map_location, 1);
            // Texture unit 2 metalness_map_
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, metalness_map_);
            glUniform1i(metalness_map_location, 2);
            // Texture unit 3 brdfLUT_map_
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, brdfLUT_map_);
            glUniform1i(brdfLUT_map_location, 3);

            // Set the Albedo value of the sphere
            Eigen::Vector3f albedo(1.0f, 1.0f, 1.0f);

            // Other Uniforms
            glUniform1i(current_text_location, currentTexture_);
            glUniform3f(light_location, 2.0f, 2.0f, 5.0f);
            glUniform3f(camera_location, 0.0f, 0.0f, 0.0f);
            glUniform3f(fresnel_location, fresnel_[0], fresnel_[1], fresnel_[2]);
            glUniform1f(roughness_location, roughness_);
            glUniform1f(metalness_location, metalness_);
            glUniform3f(albedo_location, albedo[0], albedo[1], albedo[2]);

            // Mesh draw call
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, mesh_->faces_.size(), GL_UNSIGNED_INT, (GLvoid*)0);
            glBindVertexArray(0);
            
            // END.

            // Camera debugging
            //GLfloat position[4];  // Create an array to store the camera position
            //glGetFloatv(GL_MODELVIEW_MATRIX, position);   // Get the modelview matrix and store it in position[]
            //Print the camera's position (there are the "slots" that OpenGL stores the camera's position
            //std::cout << "Camera position: (" << position[12] << ", " << position[13] << ", " << position[14] << ")" << std::endl;


            //SKY-----------------------------------------------------------------------------------------
            if(skyVisible_) {
                model = camera_.SetIdentity();

                programs_[programs_.size()-1]->bind();

                projection_location     = programs_[programs_.size()-1]->uniformLocation("projection");
                view_location           = programs_[programs_.size()-1]->uniformLocation("view");
                model_location          = programs_[programs_.size()-1]->uniformLocation("model");
                normal_matrix_location  = programs_[programs_.size()-1]->uniformLocation("normal_matrix");
                specular_map_location   = programs_[programs_.size()-1]->uniformLocation("specular_map");

                glUniformMatrix4fv(projection_location, 1, GL_FALSE, projection.data());
                glUniformMatrix4fv(view_location, 1, GL_FALSE, view.data());
                glUniformMatrix4fv(model_location, 1, GL_FALSE, model.data());
                glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, normal.data());

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
                glUniform1i(specular_map_location, 0);

                glBindVertexArray(VAO_sky);
                glDrawElements(GL_TRIANGLES, skyFaces_.size(), GL_UNSIGNED_INT, (GLvoid*)0);
                glBindVertexArray(0);

            }
        }
    }
}

void GLWidget::SetReflection(bool set) {
    if(set) currentShader_ = 2;
    updateGL();
}

void GLWidget::SetPBS(bool set) {
    if(set) currentShader_ = 3;
    updateGL();
}

void GLWidget::SetIBLPBS(bool set) {
    if(set) currentShader_ = 4;
    updateGL();
}

void GLWidget::SetPhong(bool set)
{
    if(set) currentShader_ = 0;
    updateGL();
}

void GLWidget::SetTexMap(bool set)
{
    if(set) currentShader_ = 1;
    updateGL();
}

void GLWidget::SetFresnelR(double r) {
  fresnel_[0] = r;
  updateGL();
}

void GLWidget::SetFresnelG(double g) {
  fresnel_[1] = g;
  updateGL();
}

void GLWidget::SetFresnelB(double b) {
  fresnel_[2] = b;
  updateGL();
}

void GLWidget::SetCurrentTexture(int i)
{
    currentTexture_ = i;
    updateGL();
}

void GLWidget::SetSkyVisible(bool set)
{
    skyVisible_ = set;
}

void GLWidget::SetMetalness(double d) {
    metalness_ = d;
    updateGL();
}

void GLWidget::SetRoughness(double d) {
    roughness_ = d;
    updateGL();
}
