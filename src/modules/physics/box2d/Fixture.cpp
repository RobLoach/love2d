/**
* Copyright (c) 2006-2011 LOVE Development Team
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
**/

#include "Fixture.h"

// Module
#include "Body.h"
#include "World.h"
#include "Physics.h"

#include <common/Memoizer.h>

// STD
#include <bitset>

namespace love
{
namespace physics
{
namespace box2d
{
	Fixture::Fixture(Body * body, Shape * shape, float density)
		: body(body), fixture(NULL)
	{
		body->retain();
		data = new fixtureudata();
		data->ref = 0;
		b2FixtureDef def;
		def.shape = shape->shape;
		def.userData = (void *)data;
		def.density = density;
		fixture = body->body->CreateFixture(&def);
		shape = new Shape(fixture->GetShape());
		Memoizer::add(fixture, this);
	}
	
	Fixture::Fixture(b2Fixture * f)
		: fixture(f)
	{
		data = (fixtureudata *)f->GetUserData();
		body = (Body *)Memoizer::find(f->GetBody());
		if (!body) body = new Body(f->GetBody());
		body->retain();
		shape = (Shape *)Memoizer::find(f->GetShape());
		if (!shape) shape = new Shape(f->GetShape());
		else shape->retain();
		Memoizer::add(fixture, this);
	}

	Fixture::~Fixture()
	{
		if(data->ref != 0)
			delete data->ref;

		delete data;
		data = 0;

		if (fixture)
			body->body->DestroyFixture(fixture);
		fixture = 0;

		body->release();
		shape->release();
	}

	Shape::Type Fixture::getType() const
	{
		return shape->getType();
	}

	void Fixture::setFriction(float friction)
	{
		fixture->SetFriction(friction);
	}

	void Fixture::setRestitution(float restitution)
	{
		fixture->SetRestitution(restitution);
	}

	void Fixture::setDensity(float density)
	{
		fixture->SetDensity(density);
	}

	void Fixture::setSensor(bool sensor)
	{
		fixture->SetSensor(sensor);
	}

	float Fixture::getFriction() const
	{
		return fixture->GetFriction();
	}

	float Fixture::getRestitution() const
	{
		return fixture->GetRestitution();
	}

	float Fixture::getDensity() const
	{
		return fixture->GetDensity();
	}

	bool Fixture::isSensor() const
	{
		return fixture->IsSensor();
	}

	Body * Fixture::getBody() const
	{
		return body;
	}
	
	Shape * Fixture::getShape() const
	{
		return shape;
	}
	
	Fixture * Fixture::getNext() const
	{
		b2Fixture * f = fixture->GetNext();
		if (!f) return NULL;
		Fixture * fix = (Fixture *)Memoizer::find(f);
		if (!fix) fix = new Fixture(f);
		return fix;
	}

	void Fixture::setFilterData(int * v)
	{
		b2Filter f;
		f.categoryBits = (unsigned short)v[0];
		f.maskBits = (unsigned short)v[1];
		f.groupIndex = v[2];
		fixture->SetFilterData(f);
	}

	void Fixture::getFilterData(int * v)
	{
		b2Filter f = fixture->GetFilterData();
		v[0] = (int)f.categoryBits;
		v[1] = (int)f.maskBits;
		v[2] = f.groupIndex;
	}

	int Fixture::setCategory(lua_State * L)
	{
		b2Filter f = fixture->GetFilterData();
		f.categoryBits = (uint16)getBits(L);
		fixture->SetFilterData(f);
		return 0;
	}

	int Fixture::setMask(lua_State * L)
	{
		b2Filter f = fixture->GetFilterData();
		f.maskBits = ~(uint16)getBits(L);
		fixture->SetFilterData(f);
		return 0;
	}

	void Fixture::setGroupIndex(int index)
	{
		b2Filter f = fixture->GetFilterData();
		f.groupIndex = (uint16)index;
		fixture->SetFilterData(f);
	}

	int Fixture::getGroupIndex() const
	{
		b2Filter f = fixture->GetFilterData();
		return f.groupIndex;
	}

	int Fixture::getCategory(lua_State * L)
	{
		return pushBits(L, fixture->GetFilterData().categoryBits);
	}

	int Fixture::getMask(lua_State * L)
	{
		return pushBits(L, ~(fixture->GetFilterData().maskBits));
	}

	uint16 Fixture::getBits(lua_State * L)
	{
		// Get number of args.
		int argc = lua_gettop(L);

		// The new bitset.
		std::bitset<16> b;

		for(int i = 1;i<=argc;i++)
		{
			size_t bpos = (size_t)(lua_tointeger(L, i)-1);
			if(bpos > 16)
				return luaL_error(L, "Values must be in range 1-16.");
			b.set(bpos, true);
		}

		return (uint16)b.to_ulong();
	}

	int Fixture::pushBits(lua_State * L, uint16 bits)
	{
		// Create a bitset.
		std::bitset<16> b((int)bits);

		// Push all set bits.
		for(int i = 0;i<16;i++)
			if(b.test(i))
				lua_pushinteger(L, i+1);

		// Count number of set bits.
		return (int)b.count();
	}

	int Fixture::setUserData(lua_State * L)
	{
		love::luax_assert_argc(L, 1, 1);

		if(data->ref != 0)
		{
			delete data->ref;
			data->ref = 0;
		}

		data->ref = new Reference(L);
		return 0;
	}

	int Fixture::getUserData(lua_State * L)
	{
		love::luax_assert_argc(L, 0, 0);
		if(data->ref != 0)
			data->ref->push();
		else
			lua_pushnil(L);

		return 1;
	}
	
	bool Fixture::testPoint(float x, float y) const
	{
		return fixture->TestPoint(Physics::scaleDown(b2Vec2(x, y)));
	}
	
	int Fixture::rayCast(lua_State * L) const
	{
		float p1x = Physics::scaleDown((float)luaL_checknumber(L, 1));
		float p1y = Physics::scaleDown((float)luaL_checknumber(L, 2));
		float p2x = Physics::scaleDown((float)luaL_checknumber(L, 3));
		float p2y = Physics::scaleDown((float)luaL_checknumber(L, 4));
		float maxFraction = (float)luaL_checknumber(L, 5);
		float x = Physics::scaleDown((float)luaL_checknumber(L, 6));
		float y = Physics::scaleDown((float)luaL_checknumber(L, 7));
		float r = (float)luaL_checknumber(L, 8);
		int childIndex = (int)luaL_optint(L, 9, 0);
		b2RayCastInput input;
		input.p1.Set(p1x, p1y);
		input.p2.Set(p2x, p2y);
		input.maxFraction = maxFraction;
		b2Transform transform(b2Vec2(x, y), b2Rot(r));
		b2RayCastOutput output;
		fixture->RayCast(&output, input, childIndex);
		lua_pushnumber(L, Physics::scaleUp(output.normal.x));
		lua_pushnumber(L, Physics::scaleUp(output.normal.y));
		lua_pushnumber(L, output.fraction);
		return 3;
	}
	
	int Fixture::getBoundingBox(lua_State * L) const
	{
		int childIndex = (int)luaL_optint(L, 1, 0);
		b2AABB box = fixture->GetAABB(childIndex);
		box = Physics::scaleUp(box);
		lua_pushnumber(L, box.lowerBound.x);
		lua_pushnumber(L, box.lowerBound.y);
		lua_pushnumber(L, box.upperBound.x);
		lua_pushnumber(L, box.upperBound.y);
		return 4;
	}
	
	int Fixture::getMassData(lua_State * L) const
	{
		b2MassData data;
		fixture->GetMassData(&data);
		b2Vec2 center = Physics::scaleUp(data.center);
		lua_pushnumber(L, center.x);
		lua_pushnumber(L, center.y);
		lua_pushnumber(L, data.mass);
		lua_pushnumber(L, data.I);
		return 4;
	}
	
	
} // box2d
} // physics
} // love
