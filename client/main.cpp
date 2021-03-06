/*
 * QDigiDocClient
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

#ifdef BREAKPAD
#include <common/QBreakPad.h>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#endif

int main( int argc, char *argv[] )
{
#ifdef BREAKPAD
	if( QBreakPad::isCrashReport( argc, argv ) )
	{
		Common app( argc, argv, APP, ":/images/digidoc_icon_128x128.png" );

		QBreakPadDialog d( app.applicationName() );
		d.setProperty( "User-Agent", QString( "%1/%2 (%3)" )
			.arg( app.applicationName(), app.applicationVersion(), app.applicationOs() ).toUtf8() );
		d.show();
		return app.exec();
	}
	QBreakPad breakpad;
#endif

	Application a( argc, argv );
	return a.run();
}
