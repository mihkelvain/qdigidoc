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

#pragma once

#include <common/Common.h>

#include <QtCore/QStringList>
#include <QtCore/QVariant>

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application*>(QCoreApplication::instance()))

namespace digidoc { class Exception; }
class QAction;
class QSigner;
class ApplicationPrivate;
class Application: public Common
{
	Q_OBJECT

public:
	enum ConfParameter
	{
		PKCS11Module,
		ProxyHost,
		ProxyPort,
		ProxyUser,
		ProxyPass,
		PKCS12Cert,
		PKCS12Pass,
		PKCS12Disable,
		TSLUrl,
		TSLCert,
		TSLCache,
		TSLOnlineDigest
	};

	explicit Application( int &argc, char **argv );
	~Application();

	void loadTranslation( const QString &lang );
	bool notify( QObject *o, QEvent *e );
	QSigner* signer() const;
	int run();
	void waitForTSL( const QString &file );

	static QVariant confValue( ConfParameter parameter, const QVariant &value = QVariant() );
	static void setConfValue( ConfParameter parameter, const QVariant &value );
	static void showWarning( const QString &msg, int err, const QString &details = QString(), const QString &search = QString() );

public Q_SLOTS:
	void showAbout();
	void showClient( const QStringList &params = QStringList() );
	void showCrypto( const QStringList &params = QStringList() );
	void showSettings( int page = 0, const QString &path = QString() );
	void showWarning( const QString &msg, const QString &details = QString() );

private Q_SLOTS:
	void closeWindow();
	void parseArgs( const QString &msg = QString() );
	void parseArgs( const QStringList &args );
	void showTSLWarning( QEventLoop *e );

Q_SIGNALS:
	void TSLLoadingFinished();

private:
	void activate( QWidget *w );
	bool event( QEvent *e );
	void showWarning( const QString &msg, const digidoc::Exception &e );

	QHash<QString,QString> urls() const;

	ApplicationPrivate *d;
};
