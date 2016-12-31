#include "tst_global.h"

class RestReplyTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void testReplyWrapping_data();
	void testReplyWrapping();
	void testReplyError();
	void testReplyRetry();

private:
	QNetworkAccessManager *nam;
};

void RestReplyTest::initTestCase()
{
	nam = new QNetworkAccessManager(this);
}

void RestReplyTest::cleanupTestCase()
{
	nam->deleteLater();
	nam = nullptr;
}

void RestReplyTest::testReplyWrapping_data()
{
	QTest::addColumn<QUrl>("url");
	QTest::addColumn<bool>("succeed");
	QTest::addColumn<int>("status");
	QTest::addColumn<QJsonObject>("result");

	QJsonObject object;
	object["userId"] = 1;
	object["id"] = 1;
	object["title"] = "sunt aut facere repellat provident occaecati excepturi optio reprehenderit";
	object["body"] = "quia et suscipit\n"
					 "suscipit recusandae consequuntur expedita et cum\n"
					 "reprehenderit molestiae ut ut quas totam\n"
					 "nostrum rerum est autem sunt rem eveniet architecto";

	QTest::newRow("get") << QUrl("https://jsonplaceholder.typicode.com/posts/1")
						 << true
						 << 200
						 << object;

	QTest::newRow("get") << QUrl("https://jsonplaceholder.typicode.com/posts/baum")
						 << false
						 << 404
						 << QJsonObject();
}

void RestReplyTest::testReplyWrapping()
{
	QFETCH(QUrl, url);
	QFETCH(bool, succeed);
	QFETCH(int, status);
	QFETCH(QJsonObject, result);

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;

	auto reply = new QtRestClient::RestReply(nam->get(request));
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QJsonObject data){
		called = true;
		QVERIFY(succeed);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QCOMPARE(data, result);
	});
	reply->onFailed([&](QtRestClient::RestReply *rep, int code, QJsonObject data){
		called = true;
		QVERIFY(!succeed);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QCOMPARE(data, result);
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testReplyError()
{
	QNetworkRequest request(QStringLiteral("https://invalid.jsonplaceholder.typicode.com"));
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;

	auto reply = new QtRestClient::RestReply(nam->get(request));
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *, int, QJsonObject){
		called = true;
		QFAIL("succeed with non existant domain");
	});
	reply->onFailed([&](QtRestClient::RestReply *, int, QJsonObject){
		called = true;
		QFAIL("succeed with non existant domain");
	});
	reply->onError([&](QtRestClient::RestReply *rep, QString, int code, QtRestClient::RestReply::ErrorType type) {
		called = true;
		QCOMPARE(rep, reply);
		QCOMPARE(code, (int)QNetworkReply::HostNotFoundError);
		QCOMPARE(type, QtRestClient::RestReply::NetworkError);
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testReplyRetry()
{
	QNetworkRequest request(QStringLiteral("https://invalid.jsonplaceholder.typicode.com"));
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	auto retryCount = 0;

	auto reply = new QtRestClient::RestReply(nam->get(request));
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *, int, QJsonObject){
		retryCount = 42;
		QFAIL("succeed with non existant domain");
	});
	reply->onFailed([&](QtRestClient::RestReply *, int, QJsonObject){
		retryCount = 42;
		QFAIL("succeed with non existant domain");
	});
	reply->onError([&](QtRestClient::RestReply *rep, QString, int code, QtRestClient::RestReply::ErrorType type) {
		retryCount++;
		QCOMPARE(rep, reply);
		QCOMPARE(code, (int)QNetworkReply::HostNotFoundError);
		QCOMPARE(type, QtRestClient::RestReply::NetworkError);
		if(retryCount < 3)
			rep->retryAfter((retryCount - 1) * 1500);//first 0, the 1500
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(!deleteSpy.wait(1000));
	QVERIFY(deleteSpy.wait(14000));
	QVERIFY(retryCount);
	QCOMPARE(retryCount, 3);
}

QTEST_MAIN(RestReplyTest)

#include "tst_restreply.moc"
