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

#pragma once

#include <common/Common.h>

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application*>(QCoreApplication::instance()))

class QAction;
class Poller;
class ApplicationPrivate;
class Application: public Common
{
	Q_OBJECT

public:
	explicit Application( int &argc, char **argv );
	~Application();

	QString lastPath() const;
	void loadTranslation( const QString &lang );
	Poller* poller() const;
	void setLastPath( const QString &path );

public Q_SLOTS:
	void showAbout();
	void showSettings();
	void showWarning( const QString &msg );

private Q_SLOTS:
	void closeWindow();
	void parseArgs( const QString &msg = QString() );

private:
	bool event( QEvent *e );

	ApplicationPrivate *d;
};
