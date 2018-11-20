#include "include\Toy.h"
#include "include\Mathlib.h"

constexpr float pi = static_cast<float>(3.14159265358979323846264338327950288);
constexpr float half_pi = pi / 2.0f;
static const toy::fvec3 X_Axis(1.0f, 0.0f, 0.0f);
static const toy::fvec3 Y_Axis(0.0f, 1.0f, 0.0f);
static const toy::fvec3 Z_Axis(0.0f, 0.0f, 1.0f);

toy::IShaderProgram* prog;
toy::IShaderProgram* aprog;
toy::Camera *cam;
constexpr float camsp = 0.1f;
constexpr float camrs = pi / 180 * 3;

void reflectHID(toy::ToyEngine* eng, toy::s64 ftime)
{
	auto hid = eng->getHIDAdapter();
	if (hid->keyboardAdapter.isKeyDown('A'))
		cam->moveRight(-camsp);
	if (hid->keyboardAdapter.isKeyDown('S'))
		cam->moveForward(-camsp);
	if (hid->keyboardAdapter.isKeyDown('D'))
		cam->moveRight(camsp);
	if (hid->keyboardAdapter.isKeyDown('W'))
		cam->moveForward(camsp);
	if (hid->keyboardAdapter.isKeyDown('Q'))
		cam->moveUp(camsp);
	if (hid->keyboardAdapter.isKeyDown('E'))
		cam->moveUp(-camsp);
	if (hid->keyboardAdapter.isKeyDown('J'))
		cam->rotateRight(-camrs);
	if (hid->keyboardAdapter.isKeyDown('L'))
		cam->rotateRight(camrs);
	if (hid->keyboardAdapter.isKeyDown('I'))
		cam->rotateUp(camrs);
	if (hid->keyboardAdapter.isKeyDown('K'))
		cam->rotateUp(-camrs);
	if (hid->keyboardAdapter.isKeyDown('U'))
		cam->rotateClockwise(-camrs);
	if (hid->keyboardAdapter.isKeyDown('O'))
		cam->rotateClockwise(camrs);
	if (hid->keyboardAdapter.getKeyPress('Z'))
		cam->setZoom(cam->getZoom()*1.1f);
	if (hid->keyboardAdapter.getKeyPress('X'))
		cam->setZoom(cam->getZoom()*0.9f);
	if (hid->keyboardAdapter.getKeyPress('R'))
	{
		*cam = toy::Camera();
		cam->moveUp(1.0f);
	}
}

void loadScene(toy::SceneManager* smgr)
{
	auto scene = smgr->newScene("TestScene");

	toy::io::setCWD("D:/Projects/Models/");
	auto bikini = scene->loadModel("Bikini_girl\\bikini.fbx", true);
	bikini->prog = aprog;

	auto neptune = scene->loadModel("Neptune\\Neptune.FBX", true);
	neptune->prog = prog;

	auto bikiniIns = scene->newPrefabInstances(
		bikini, 1, toy::SceneObjectProperty::SOF_ANIMATED);
	toy::sqtMat(
		toy::fvec3(0.02f),
		toy::fquat(0.0f, toy::fvec3(1.0f, 0.0f, 0.0f)),
		toy::fvec3(-1.0f),
		bikiniIns->mmat);

	auto neptuneIns = scene->newPrefabInstances(
		neptune, 1, toy::SceneObjectProperty::SOF_VISIBLE);
	toy::fquat q = toy::fquat(-half_pi, X_Axis) * toy::fquat(pi, Z_Axis);
	toy::sqtMat(
		toy::fvec3(1.0f),
		q,
		toy::fvec3(1.0f),
		neptuneIns->mmat);

	scene->sort();

	smgr->endLoadScene(scene);
	smgr->switchScene(scene);
}

int main(int argc, char* argv[])
{
	toy::ToyEngine eng;
	auto err = eng.createDevice("Test title", 800, 600, toy::rd::RDT_OPENGL);
	assert(0 == err);

	auto lua = eng.getLuaInterpreter();

	//lua->call("print_r", "gs", "toy", "toy");
	lua->call("print_r", "gs", "glsl", "glsl");

	auto wnd = eng.getWindow();
	auto hid = eng.getHIDAdapter();
	auto sceneMgr = eng.getSceneManager();
	auto rdr = eng.getRenderDriver();
	auto smgr = eng.getSceneManager();

	rdr->enableFunction(toy::rd::RF_DEPTH_TEST, true);
	rdr->enableFunction(toy::rd::RF_VSYNC, false);
	
	prog = rdr->getShaderProgram(0);
	aprog = rdr->getShaderProgram(1);

	cam = smgr->newCamera();
	cam->moveUp(1.0f);
	
	loadScene(smgr);

	eng.setHIDReflection(reflectHID);
	eng.mainLoop();

	eng.destroy();

	//system("pause");
	return 0;
}