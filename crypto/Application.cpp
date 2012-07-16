/*
 * QDigiDocCrypto
 *
 * Copyright (C) 2010-2012 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010-2012 Raul Metsma <raul@innovaatik.ee>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "Application.h"

#include "MainWindow.h"
#include "Poller.h"
#include "SettingsDialog.h"
#include "version.h"

#include <common/AboutWidget.h>
#include <common/Settings.h>

#include <libdigidoc/DigiDocConfig.h>

#include "qtsingleapplication/src/qtlocalpeer.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTranslator>
#include <QtCore/QUrl>
#include <QtGui/QFileOpenEvent>
#include <QtGui/QMessageBox>

#if defined(Q_OS_MAC)
#include <common/MacMenuBar.h>
#endif

class ApplicationPrivate
{
public:
	ApplicationPrivate(): bar(0), poller(0) {}

	QAction		*closeAction, *newAction;
#ifdef Q_OS_MAC
	MacMenuBar	*bar;
#endif
	Poller		*poller;
	QTranslator	*appTranslator, *commonTranslator, *qtTranslator;
	QString		lang;
};

Application::Application( int &argc, char **argv )
:	Common( argc, argv )
,	d( new ApplicationPrivate )
{
	QStringList args = arguments();
	args.removeFirst();
	if( isRunning() )
	{
		sendMessage( args.join( "\", \"" ) );
		return;
	}
	connect( this, SIGNAL(messageReceived(QString)), SLOT(parseArgs(QString)) );

	setApplicationName( APP );
	setApplicationVersion( VER_STR( FILE_VER_DOT ) );
	setOrganizationDomain( DOMAINURL );
	setOrganizationName( ORG );
	setWindowIcon( QIcon( ":/images/crypto_128x128.png" ) );

	// Actions
	d->closeAction = new QAction( this );
	d->closeAction->setShortcut( Qt::CTRL + Qt::Key_W );
	connect( d->closeAction, SIGNAL(triggered()), SLOT(closeWindow()) );

	d->newAction = new QAction( this );
	d->newAction->setShortcut( Qt::CTRL + Qt::Key_N );
	connect( d->newAction, SIGNAL(triggered()), SLOT(parseArgs()) );

#if defined(Q_OS_MAC)
	setQuitOnLastWindowClosed( false );

	d->bar = new MacMenuBar;
	d->bar->addAction( MacMenuBar::AboutAction, this, SLOT(showAbout()) );
	d->bar->addAction( MacMenuBar::PreferencesAction, this, SLOT(showSettings()) );
	d->bar->fileMenu()->addAction( d->newAction );
	d->bar->fileMenu()->addAction( d->closeAction );
	d->bar->dockMenu()->addAction( d->newAction );
#endif

	installTranslator( d->appTranslator = new QTranslator( this ) );
	installTranslator( d->commonTranslator = new QTranslator( this ) );
	installTranslator( d->qtTranslator = new QTranslator( this ) );
	loadTranslation( Settings::language() );

	initDigiDocLib();
	QString ini = QString( "%1/digidoc.ini" ).arg( applicationDirPath() );
	if( QFileInfo( ini ).isFile() )
		initConfigStore( ini.toUtf8() );
	else
		initConfigStore( NULL );

#if 0 //def Q_OS_WIN
	Poller::ApiType api = QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA ? Poller::CAPI : Poller::PKCS11;
#else
	Poller::ApiType api = Poller::PKCS11;
#endif
	if( args.contains("-capi") ) api = Poller::CAPI;
	if( args.contains("-pkcs11") ) api = Poller::PKCS11;
	if( args.contains("-cng") ) api = Poller::CNG;
	d->poller = new Poller( api, this );
	parseArgs( args );
}

Application::~Application()
{
	if( !isRunning() )
	{
		if( QtLocalPeer *obj = findChild<QtLocalPeer*>() )
			delete obj;
#ifdef Q_OS_MAC
		delete d->bar;
#endif
		cleanupConfigStore( NULL );
		finalizeDigiDocLib();
	}
	delete d;
}

void Application::closeWindow()
{
#ifndef Q_OS_MAC
	if( MainWindow *w = qobject_cast<MainWindow*>(activeWindow()) )
		w->closeDoc();
	else
#endif
	if( QDialog *d = qobject_cast<QDialog*>(activeWindow()) )
		d->reject();
	else if( QWidget *w = qobject_cast<QWidget*>(activeWindow()) )
		w->close();
}

bool Application::event( QEvent *e )
{
	switch( int(e->type()) )
	{
	case REOpenEvent::Type:
		if( !activeWindow() )
			parseArgs();
		return true;
	case OpenFilesEvent::Type:
		parseArgs( static_cast<OpenFilesEvent*>(e)->files() );
		return true;
	case QEvent::FileOpen:
		parseArgs( QStringList() << static_cast<QFileOpenEvent*>(e)->file() );
		return true;
	default: return Common::event( e );
	}
}

QString Application::lastPath() const
{ return Settings().value( "Crypto/lastPath" ).toString(); }

void Application::loadTranslation( const QString &lang )
{
	if( d->lang == lang )
		return;
	Settings().setValue( "Main/Language", d->lang = lang );

	if( lang == "en" ) QLocale::setDefault( QLocale( QLocale::English, QLocale::UnitedKingdom ) );
	else if( lang == "ru" ) QLocale::setDefault( QLocale( QLocale::Russian, QLocale::RussianFederation ) );
	else QLocale::setDefault( QLocale( QLocale::Estonian, QLocale::Estonia ) );

	d->appTranslator->load( ":/translations/" + lang );
	d->commonTranslator->load( ":/translations/common_" + lang );
	d->qtTranslator->load( ":/translations/qt_" + lang );
	d->closeAction->setText( tr("Close window") );
	d->newAction->setText( tr("New window") );
}

void Application::parseArgs( const QString &msg )
{
	QStringList params;
	Q_FOREACH( const QString &param, msg.split( "\", \"", QString::SkipEmptyParts ) )
	{
		QUrl url( param );
		params << (url.errorString().isEmpty() ? url.toLocalFile() : param);
	}
	parseArgs( params );
}

void Application::parseArgs( const QStringList &args )
{
	QStringList params = args;
	params.removeAll("-capi");
	params.removeAll("-cng");
	params.removeAll("-pkcs11");
	params.removeAll("-noNativeFileDialog");

	QWidget *w = new MainWindow();
#ifdef Q_OS_MAC
	w->installEventFilter( d->bar );
#endif
	w->addAction( d->closeAction );
	w->activateWindow();
	w->show();
	w->raise();
	if( !params.isEmpty() )
		QMetaObject::invokeMethod( w, "open", Q_ARG(QStringList,params) );
}

Poller* Application::poller() const { return d->poller; }

void Application::setLastPath( const QString &path )
{ Settings().setValue( "Crypto/lastPath", path ); }

void Application::showAbout()
{
	AboutWidget *a = new AboutWidget( activeWindow() );
	a->addAction( d->closeAction );
	a->show();
}

void Application::showSettings()
{
	SettingsDialog *s = new SettingsDialog( activeWindow() );
	s->addAction( d->closeAction );
	s->show();
}

void Application::showWarning( const QString &msg )
{ QMessageBox( QMessageBox::Warning, tr("DigiDoc3 crypto"), msg, QMessageBox::Close, activeWindow() ).exec(); }
