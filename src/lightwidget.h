#ifndef LIGHTWIDGET_H
#define LIGHTWIDGET_H

#include <QWidget>

class OgreManager;


namespace Ogre
{
class Root;
class SceneManager;
class Light;
}

namespace Ui {
class LightWidget;
}

class LightWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LightWidget(QWidget* parent = 0);
    ~LightWidget();

	void init(OgreManager*);
	void connections();

protected:
	void hideEvent( QHideEvent* evt ) override;

private:
	Ogre::Light* findLightByName( std::string lightName );
	
	bool loadFromSettings();
	void writeToSettings();

	void createLight( QString name, QMap<QString, QVariant> attri );
	void saveLight( Ogre::Light* light );
	void createDefaultLights();

	// slots
	void currentLightChanged();
	void positionChanged( double v );
	void directionChanged( double v );
	void colorChanged();
	void typeChanged( int index );
	void powerScaleChanged( float v );
	void addLight();
	void removeLight();
	void lightAttenuationChanged();
	void lightAngleChanged();


private:
    Ui::LightWidget *ui;

	Ogre::SceneManager* mSceneManager = nullptr;
	Ogre::Root* mRoot = nullptr;

	Ogre::Light* mCurrentLight = nullptr;
};

#endif // LIGHTWIDGET_H
