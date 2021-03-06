#ifndef CLASSBUILDER_H
#define CLASSBUILDER_H

#include "restbuilder.h"
#include <QVersionNumber>
#if QT_HAS_INCLUDE(<variant>) && __cplusplus >= 201703L
#include <variant>
#endif

class ClassBuilder : public RestBuilder
{
public:
	explicit ClassBuilder(QXmlStreamReader &reader);

	static bool canReadType(const QString &type);

private:
	struct Expression {
		bool expr = false;
		QString value;
	};

	struct FixedParam : public Expression {
		QString key;
	};

	struct RestAccess {
		QString name;
		QString exportKey;
		QString nspace;
		QString qmlUri;
		QString base;
		QString except;

		QList<Include> includes;

		struct Class {
			QString key;
			QString type;
		};
		QList<Class> classes;

		struct Method {
			QString name;
			QString verb;
			QString body;
			QString returns;
			QString except;
			bool postParams = false;

#if QT_HAS_INCLUDE(<variant>) && __cplusplus >= 201703L
			using PathInfoBase = std::variant<Expression, BaseParam>;
			using PathInfoList = QList<PathInfoBase>;
			using PathInfo = std::variant<PathInfoList, QString>;
#else
			struct PathInfoBase {
				bool isParams = false;
				Expression path;
				BaseParam pathParams;

				inline PathInfoBase() = default;
				inline PathInfoBase(const PathInfoBase &other) = default;

				inline PathInfoBase(Expression expr) :
					isParams{false},
					path{std::move(expr)}
				{}
				inline PathInfoBase(BaseParam param) :
					isParams{true},
					pathParams{std::move(param)}
				{}
			};
			using PathInfoList = QList<PathInfoBase>;
			struct PathInfo {
				bool isPath = true;
				PathInfoList path;
				QString url;

				inline PathInfo &operator=(PathInfoList &&list) {
					path = list;
					isPath = true;
					return *this;
				}
				inline PathInfo &operator=(QString &&str) {
					url = str;
					isPath = false;
					return *this;
				}
			};
#endif
			PathInfo path;
			QList<BaseParam> params;
			QList<FixedParam> headers;
		};
		QList<Method> methods;
	} data;

	struct RestClass {
		Expression path;
	} classData;

	struct RestApi {
		QString globalName;
		bool autoCreate = false;

		Expression baseUrl;
		QVersionNumber apiVersion;
		QList<FixedParam> params;
		QList<FixedParam> headers;
	} apiData;

	bool isApi = false;

	void build() override;

	void readData();
	Expression readExpression();
	FixedParam readFixedParam();
	RestAccess::Class readClass();
	RestAccess::Method readMethod();

	bool hasNs();
	bool hasQml();

	void generateClass();
	void generateApi();

	void writeClassBeginDeclaration();
	void writeClassMainDeclaration();
	void writeClassBeginDefinition();
	void writeClassMainDefinition();

	QString writeExpression(const Expression &expression, bool asString);
	QString writeMethodParams(const RestAccess::Method &method, bool withDefaults);

	void writeFactoryDeclaration();
	void writePrivateDefinitions();
	void writeFactoryDefinition();
	void writeClassDefinitions();
	void writeMethodDefinitions();
	void writeMemberDefinitions();
	void writeStartupCode();

	void writeLocalApiGeneration();
	void writeGlobalApiGeneration();
	void writeApiCreation();

	void writeQmlDeclaration();
	void writeQmlDefinitions();

	bool writeMethodPath(const ClassBuilder::RestAccess::Method::PathInfo &info);

#if QT_HAS_INCLUDE(<variant>) && __cplusplus >= 201703L
	template <typename TVariant, typename TInfo>
	static constexpr TVariant &get(TInfo &info) {
		return std::get<TVariant>(info);
	}
	template <typename TVariant, typename TInfo>
	static constexpr const TVariant &get(const TInfo &info) {
		return std::get<TVariant>(info);
	}
	template <typename TVariant, typename TInfo>
	static constexpr bool is(const TInfo &info) {
		return std::holds_alternative<TVariant>(info);
	}
#else
	template <typename TVariant, typename TInfo>
	static TVariant &get(TInfo &info);
	template <typename TVariant, typename TInfo>
	static const TVariant &get(const TInfo &info);
	template <typename TVariant, typename TInfo>
	static constexpr bool is(const TInfo &info);
#endif
};

#endif // CLASSBUILDER_H
