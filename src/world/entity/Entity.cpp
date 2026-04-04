#include "Entity.h"
#include "EntityPos.h"
#include "../level/Level.h"
#include "../level/tile/LiquidTile.h"
#include "item/ItemEntity.h"
#include "../item/ItemInstance.h"
#include "../../nbt/CompoundTag.h"
#include "../../util/PerfTimer.h"

int Entity::entityCounter = 0;
Random Entity::sharedRandom(getEpochTimeS());

Entity::Entity(Level* level)
:	level(level),
	viewScale(1.0f),
	blocksBuilding(false),
	onGround(false),
	wasInWater(false),
	collision(false),
	hurtMarked(false),
	slide(true),
	isStuckInWeb(false),
	removed(false),
	reallyRemoveIfPlayer(false),
	canRemove(true),
	noPhysics(false),
	firstTick(true),

	bbWidth(0.6f),
	bbHeight(1.8f),
	heightOffset(0 / 16.0f),
	bb(0,0,0,0,0,0),

	ySlideOffset(0),
	fallDistance(0),
	footSize(0),
	invulnerableTime(0),
	pushthrough(0),
	airCapacity(TOTAL_AIR_SUPPLY),
	airSupply(TOTAL_AIR_SUPPLY),

	xOld(0), yOld(0), zOld(0),
	horizontalCollision(false), verticalCollision(false),

	x(0), y(0), z(0),
	xo(0), yo(0), zo(0), xd(0), yd(0), zd(0),
	xRot(0), yRot(0),
	xRotO(0), yRotO(0),

	xChunk(0), yChunk(0), zChunk(0),
	inChunk(false),

	fireImmune(false),
	onFire(0),
	flameTime(1),
	walkDist(0), walkDistO(0),
	tickCount(0),
	entityRendererId(ER_DEFAULT_RENDERER),
	nextStep(1),
	makeStepSound(true),
	invisible(false)
{
	_init();
	entityId = ++entityCounter;
	setPos(0, 0, 0);
}

Entity::~Entity() {}

SynchedEntityData* Entity::getEntityData() { return NULL; }
const SynchedEntityData* Entity::getEntityData() const { return NULL; }

bool Entity::isInWall() {
    int xt = Mth::floor(x);
    int yt = Mth::floor(y + getHeadHeight());
    int zt = Mth::floor(z);
    return level->isSolidBlockingTile(xt, yt, zt);
}

void Entity::resetPos(bool clearMore) {
    if (level == NULL) return;
    while (y > 0) {
        setPos(x, y, z);
        if (level->getCubes(this, bb).size() == 0) break;
        y += 1;
    }
    xd = yd = zd = 0;
    xRot = 0;
}

bool Entity::isInWater() {
    return level->checkAndHandleWater(bb.grow(0, -0.4f, 0), Material::water, this);
}

bool Entity::isInLava() {
	return level->containsMaterial(bb.grow(-0.1f, -0.4f, -0.1f), Material::lava);
}

bool Entity::isFree(float xa, float ya, float za, float grow) {
    AABB box = bb.grow(grow, grow, grow).cloneMove(xa, ya, za);
    const std::vector<AABB>& aABBs = level->getCubes(this, box);
    if (aABBs.size() > 0) return false;
    if (level->containsAnyLiquid(box)) return false;
    return true;
}

bool Entity::isFree(float xa, float ya, float za) {
    AABB box = bb.cloneMove(xa, ya, za);
	const std::vector<AABB>& aABBs = level->getCubes(this, box);
    if (aABBs.size() > 0) return false;
    if (level->containsAnyLiquid(box)) return false;
    return true;
}

// 双精度移动
void Entity::move(double xa, double ya, double za) {
	if (noPhysics) {
        bb.move((float)xa, (float)ya, (float)za);
        x = (bb.x0 + bb.x1) / 2.0;
        y = bb.y0 + heightOffset - ySlideOffset;
        z = (bb.z0 + bb.z1) / 2.0;
        return;
    }

	TIMER_PUSH("move");

    double xo = x;
    double zo = z;

	if (isStuckInWeb) {
		isStuckInWeb = false;
		xa *= .25;
		ya *= .05;
		za *= .25;
		xd = 0.0;
		yd = 0.0;
		zd = 0.0;
	}

    double xaOrg = xa;
    double yaOrg = ya;
    double zaOrg = za;

    AABB bbOrg = bb;

    bool sneaking = onGround && isSneaking();

    if (sneaking) {
        float d = 0.05f;
        while (xa != 0 && level->getCubes(this, bb.cloneMove((float)xa, -1.0f, 0)).empty()) {
            if (xa < d && xa >= -d) xa = 0;
            else if (xa > 0) xa -= d;
            else xa += d;
            xaOrg = xa;
        }
        while (za != 0 && level->getCubes(this, bb.cloneMove(0, -1.0f, (float)za)).empty()) {
            if (za < d && za >= -d) za = 0;
            else if (za > 0) za -= d;
            else za += d;
            zaOrg = za;
        }
		while (xa != 0 && za != 0 && level->getCubes(this, bb.cloneMove((float)xa, -1.0f, (float)za)).empty()) {
			if (xa < d && xa >= -d) xa = 0;
			else if (xa > 0) xa -= d;
			else xa += d;
			if (za < d && za >= -d) za = 0;
			else if (za > 0) za -= d;
			else za += d;
			xaOrg = xa;
			zaOrg = za;
		}
    }

    std::vector<AABB>& aABBs = level->getCubes(this, bb.expand((float)xa, (float)ya, (float)za));

    for (unsigned int i = 0; i < aABBs.size(); i++)
        ya = aABBs[i].clipYCollide(bb, (float)ya);
    bb.move(0, (float)ya, 0);

	if (!slide && yaOrg != ya) {
		xa = ya = za = 0;
	}

    bool og = onGround || (yaOrg != ya && yaOrg < 0);

    for (unsigned int i = 0; i < aABBs.size(); i++)
        xa = aABBs[i].clipXCollide(bb, (float)xa);
    bb.move((float)xa, 0, 0);

    if (!slide && xaOrg != xa) {
        xa = ya = za = 0;
    }

    for (unsigned int i = 0; i < aABBs.size(); i++)
        za = aABBs[i].clipZCollide(bb, (float)za);
    bb.move(0, 0, (float)za);

    if (!slide && zaOrg != za) {
        xa = ya = za = 0;
    }

    if (footSize > 0 && og && (ySlideOffset < 0.05f) && ((xaOrg != xa) || (zaOrg != za))) {
		double xaN = xa;
        double yaN = ya;
        double zaN = za;
        xa = xaOrg;
        ya = footSize;
        za = zaOrg;
        AABB normal = bb;
        bb.set(bbOrg);
        aABBs = level->getCubes(this, bb.expand((float)xa, (float)ya, (float)za));

        for (unsigned int i = 0; i < aABBs.size(); i++)
            ya = aABBs[i].clipYCollide(bb, (float)ya);
        bb.move(0, (float)ya, 0);

        if (!slide && yaOrg != ya) {
            xa = ya = za = 0;
        }

		for (unsigned int i = 0; i < aABBs.size(); i++)
            xa = aABBs[i].clipXCollide(bb, (float)xa);
        bb.move((float)xa, 0, 0);

        if (!slide && xaOrg != xa) {
            xa = ya = za = 0;
        }

        for (unsigned int i = 0; i < aABBs.size(); i++)
            za = aABBs[i].clipZCollide(bb, (float)za);
        bb.move(0, 0, (float)za);

        if (!slide && zaOrg != za) {
            xa = ya = za = 0;
        }

        if (xaN * xaN + zaN * zaN >= xa * xa + za * za) {
            xa = xaN;
            ya = yaN;
            za = zaN;
            bb.set(normal);
        } else {
            ySlideOffset += 0.5f;
        }
    }

	TIMER_POP_PUSH("rest");

	x = (bb.x0 + bb.x1) / 2.0;
    y = bb.y0 + heightOffset - ySlideOffset;
    z = (bb.z0 + bb.z1) / 2.0;

    horizontalCollision = (xaOrg != xa) || (zaOrg != za);
    verticalCollision = (yaOrg != ya);
    onGround = yaOrg != ya && yaOrg < 0;
    collision = horizontalCollision || verticalCollision;
    checkFallDamage((float)ya, onGround);

    if (xaOrg != xa) xd = 0;
    if (yaOrg != ya) yd = 0;
    if (zaOrg != za) zd = 0;

    double xm = x - xo;
    double zm = z - zo;

    if (makeStepSound && !sneaking) {
        walkDist += (float)Mth::sqrt(xm * xm + zm * zm) * 0.6f;
        int xt = Mth::floor(x);
        int yt = Mth::floor(y - 0.2f - this->heightOffset);
        int zt = Mth::floor(z);
        int t = level->getTile(xt, yt, zt);
        if (t == 0) {
            int under = level->getTile(xt, yt-1, zt);
            if (Tile::fence->id == under || Tile::fenceGate->id == under) t = under;
        }
        if (walkDist > nextStep && t > 0) {
            nextStep = ((int)walkDist) + 1;
            playStepSound(xt, yt, zt, t);
        }
    }

    int x0 = Mth::floor(bb.x0);
    int y0 = Mth::floor(bb.y0);
    int z0 = Mth::floor(bb.z0);
    int x1 = Mth::floor(bb.x1);
    int y1 = Mth::floor(bb.y1);
    int z1 = Mth::floor(bb.z1);

    if (level->hasChunksAt(x0, y0, z0, x1, y1, z1)) {
        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++) {
                    int t = level->getTile(x, y, z);
                    if (t > 0) Tile::tiles[t]->entityInside(level, x, y, z, this);
                }
    }

    ySlideOffset *= 0.4f;

    bool water = this->isInWater();
    if (level->containsFireTile(bb)) {
        burn(1);
        if (!water) {
            onFire++;
            if (onFire == 0) onFire = 20 * 15;
        }
    } else {
        if (onFire <= 0) onFire = -flameTime;
    }

    if (water && onFire > 0) onFire = -flameTime;

	TIMER_POP();
}

void Entity::makeStuckInWeb() {
	isStuckInWeb = true;
	fallDistance = 0;
}

bool Entity::isUnderLiquid(const Material* material) {
    double yp = y + getHeadHeight();
    int xt = Mth::floor(x);
    int yt = Mth::floor(yp);
    int zt = Mth::floor(z);
    int t = level->getTile(xt, yt, zt);
    if (t != 0 && Tile::tiles[t]->material == material) {
        float hh = LiquidTile::getHeight(level->getData(xt, yt, zt)) - 1 / 9.0f;
        float h = yt + 1 - hh;
        return yp < h;
    }
    return false;
}

void Entity::setPos(EntityPos* pos) {
    if (pos->move) setPos(pos->x, pos->y, pos->z);
    else setPos(x, y, z);
    if (pos->rot) setRot(pos->yRot, pos->xRot);
    else setRot(yRot, xRot);
}

// 双精度 setPos
void Entity::setPos(double x, double y, double z) {
	this->x = x; this->y = y; this->z = z;
	float w = bbWidth / 2;
	float h = bbHeight;
	bb.set((float)(x - w), (float)(y - heightOffset + ySlideOffset), (float)(z - w),
	       (float)(x + w), (float)(y - heightOffset + ySlideOffset + h), (float)(z + w));
}

float Entity::getBrightness(float a) {
    int xTile = Mth::floor(x);
    float hh = (bb.y1 - bb.y0) * 0.66f;
    int yTile = Mth::floor(y - this->heightOffset + hh);
    int zTile = Mth::floor(z);
    if (level->hasChunksAt(Mth::floor(bb.x0), Mth::floor(bb.y0), Mth::floor(bb.z0),
                           Mth::floor(bb.x1), Mth::floor(bb.y1), Mth::floor(bb.z1))) {
        return level->getBrightness(xTile, yTile, zTile);
    }
    return 0;
}

bool Entity::operator==(Entity& rhs) { return entityId == rhs.entityId; }
int Entity::hashCode() { return entityId; }
void Entity::remove() { removed = true; }
void Entity::setSize(float w, float h) { bbWidth = w; bbHeight = h; }
void Entity::setRot(float yRot, float xRot) { this->yRot = yRotO = yRot; this->xRot = xRotO = xRot; }

void Entity::turn(float xo, float yo) {
	float xRotOld = xRot, yRotOld = yRot;
	yRot += xo * 0.15f;
	xRot -= yo * 0.15f;
	if (xRot < -90) xRot = -90;
	if (xRot > 90) xRot = 90;
	xRotO += xRot - xRotOld;
	yRotO += yRot - yRotOld;
}

void Entity::interpolateTurn(float xo, float yo) {
	yRot += xo * 0.15f;
	xRot -= yo * 0.15f;
	if (xRot < -90) xRot = -90;
	if (xRot > 90) xRot = 90;
}

void Entity::tick() { baseTick(); }

void Entity::baseTick() {
	TIMER_PUSH("entityBaseTick");
	tickCount++;
	walkDistO = walkDist;
	xo = x; yo = y; zo = z;
	xRotO = xRot; yRotO = yRot;
	if (isInWater()) {
		if (!wasInWater && !firstTick) {
		    float speed = (float)sqrt(xd * xd * 0.2 + yd * yd + zd * zd * 0.2) * 0.2f;
		    if (speed > 1) speed = 1;
		    level->playSound(this, "random.splash", speed, 1 + (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.4f);
		    float yt = floorf(bb.y0);
		    for (int i = 0; i < 1 + bbWidth * 20; i++) {
		        float xo = (sharedRandom.nextFloat() * 2 - 1) * bbWidth;
		        float zo = (sharedRandom.nextFloat() * 2 - 1) * bbWidth;
		        level->addParticle(PARTICLETYPE(bubble), (float)x + xo, yt + 1, (float)z + zo,
		                           (float)xd, (float)yd - sharedRandom.nextFloat() * 0.2f, (float)zd);
		    }
		}
		fallDistance = 0;
		wasInWater = true;
		onFire = 0;
	} else {
		wasInWater = false;
	}
	if (level->isClientSide) {
	    onFire = 0;
	} else {
	    if (onFire > 0) {
	        if (fireImmune) {
	            onFire -= 4;
	            if (onFire < 0) onFire = 0;
	        } else {
	            if (onFire % 20 == 0) hurt(NULL, 1);
	            onFire--;
	        }
	    }
	}
	if (isInLava()) lavaHurt();
	if (y < -64) outOfWorld();
	firstTick = false;
	TIMER_POP();
}

void Entity::outOfWorld() { remove(); }

void Entity::checkFallDamage(float ya, bool onGround) {
	if (onGround) {
		if (fallDistance > 0) {
			if(isMob()) {
				int xt = Mth::floor(x);
				int yt = Mth::floor(y - 0.2f - heightOffset);
				int zt = Mth::floor(z);
				int t = level->getTile(xt, yt, zt);
				if (t == 0 && level->getTile(xt, yt-1, zt) == Tile::fence->id) t = level->getTile(xt, yt-1, zt);
				if (t > 0) Tile::tiles[t]->fallOn(level, xt, yt, zt, this, fallDistance);
			}
			causeFallDamage(fallDistance);
			fallDistance = 0;
		}
	} else {
		if (ya < 0) fallDistance -= ya;
	}
}

void Entity::causeFallDamage(float fallDamage2) {}
float Entity::getHeadHeight() { return 0; }

void Entity::moveRelative(float xa, float za, float speed) {
	float dist = sqrt(xa*xa + za*za);
	if (dist < 0.01f) return;
	if (dist < 1) dist = 1;
	dist = speed / dist;
	xa *= dist; za *= dist;
	float sin_ = (float)sin(yRot * Mth::PI / 180);
	float cos_ = (float)cos(yRot * Mth::PI / 180);
	xd += xa * cos_ - za * sin_;
	zd += za * cos_ + xa * sin_;
}

void Entity::setLevel(Level* level) { this->level = level; }

void Entity::moveTo(double x, double y, double z, float yRot, float xRot) {
	this->xOld = this->xo = this->x = x;
	this->yOld = this->yo = this->y = y + heightOffset;
	this->zOld = this->zo = this->z = z;
	this->yRot = this->yRotO = yRot;
	this->xRot = this->xRotO = xRot;
	this->setPos(this->x, this->y, this->z);
}

float Entity::distanceTo(Entity* e) {
	double dx = x - e->x, dy = y - e->y, dz = z - e->z;
	return (float)sqrt(dx*dx + dy*dy + dz*dz);
}
float Entity::distanceTo(float x2, float y2, float z2) {
	double dx = x - x2, dy = y - y2, dz = z - z2;
	return (float)sqrt(dx*dx + dy*dy + dz*dz);
}
float Entity::distanceToSqr(float x2, float y2, float z2) {
	double dx = x - x2, dy = y - y2, dz = z - z2;
	return (float)(dx*dx + dy*dy + dz*dz);
}
float Entity::distanceToSqr(Entity* e) {
	double dx = x - e->x, dy = y - e->y, dz = z - e->z;
	return (float)(dx*dx + dy*dy + dz*dz);
}

void Entity::playerTouch(Player* player) {}

void Entity::push(Entity* e) {
	double xa = e->x - x;
	double za = e->z - z;
	double dd = Mth::absMax((float)xa, (float)za);
	if (dd >= 0.01f) {
		dd = sqrt(dd);
		xa /= dd; za /= dd;
		double pow = 1.0 / dd;
		if (pow > 1) pow = 1;
		xa *= pow; za *= pow;
		xa *= 0.05; za *= 0.05;
		xa *= 1 - pushthrough; za *= 1 - pushthrough;
		this->push((float)-xa, 0, (float)-za);
		e->push((float)xa, 0, (float)za);
	}
}
void Entity::push(float xa, float ya, float za) {
	xd += xa; yd += ya; zd += za;
}

void Entity::markHurt() { hurtMarked = true; }
bool Entity::hurt(Entity* source, int damage) { markHurt(); return false; }
void Entity::reset() { _init(); }
void Entity::_init() {
	xo = xOld = x;
	yo = yOld = y;
	zo = zOld = z;
	xRotO = xRot; yRotO = yRot;
	onFire = 0; removed = false; fallDistance = 0;
}
bool Entity::intersects(float x0, float y0, float z0, float x1, float y1, float z1) {
	return bb.intersects(x0, y0, z0, x1, y1, z1);
}
bool Entity::isPickable() { return false; }
bool Entity::isPushable() { return false; }
bool Entity::isShootable() { return false; }
void Entity::awardKillScore(Entity* victim, int score) {}
bool Entity::shouldRender(Vec3& c) {
	if (invisible) return false;
	double dx = x - c.x, dy = y - c.y, dz = z - c.z;
	double dist = dx*dx + dy*dy + dz*dz;
	return shouldRenderAtSqrDistance((float)dist);
}
bool Entity::shouldRenderAtSqrDistance(float distance) {
	float size = bb.getSize();
	size *= 64.0f * viewScale;
	return distance < size * size;
}
bool Entity::isCreativeModeAllowed() { return false; }
float Entity::getShadowHeightOffs() { return bbHeight / 2; }
bool Entity::isAlive() { return !removed; }
bool Entity::interact(Player* player) { return false; }
void Entity::lerpTo(double x, double y, double z, float yRot, float xRot, int steps) {
	setPos(x, y, z);
	setRot(yRot, xRot);
}
float Entity::getPickRadius() { return 0.1f; }
void Entity::lerpMotion(double xd, double yd, double zd) {
	this->xd = xd; this->yd = yd; this->zd = zd;
}
void Entity::animateHurt() {}
void Entity::setEquippedSlot(int slot, int item, int auxValue) {}
bool Entity::isSneaking() { return false; }
bool Entity::isPlayer() { return false; }
void Entity::lavaHurt() {
    if (!fireImmune) { hurt(NULL, 4); onFire = 30 * SharedConstants::TicksPerSecond; }
}
void Entity::burn(int dmg) { if (!fireImmune) hurt(NULL, dmg); }

bool Entity::save(CompoundTag* entityTag) {
	int id = getEntityTypeId();
    if (removed || id == 0) return false;
    entityTag->putInt("id", id);
    saveWithoutId(entityTag);
    return true;
}

void Entity::saveWithoutId(CompoundTag* entityTag) {
    ListTag* posList = new ListTag();
    posList->addDouble(x);
    posList->addDouble(y);
    posList->addDouble(z);
    entityTag->put("Pos", posList);

    ListTag* motionList = new ListTag();
    motionList->addDouble(xd);
    motionList->addDouble(yd);
    motionList->addDouble(zd);
    entityTag->put("Motion", motionList);

    entityTag->put("Rotation", ListTagFloatAdder(yRot)(xRot).tag);
    entityTag->putFloat("FallDistance", fallDistance);
    entityTag->putShort("Fire", (short)onFire);
    entityTag->putShort("Air", (short)airSupply);
    entityTag->putBoolean("OnGround", onGround);
    addAdditonalSaveData(entityTag);
}

bool Entity::load(CompoundTag* tag) {
    ListTag* pos = tag->getList("Pos");
    ListTag* motion = tag->getList("Motion");
    ListTag* rotation = tag->getList("Rotation");
    setPos(0, 0, 0);

    xd = motion->getDouble(0);
    yd = motion->getDouble(1);
    zd = motion->getDouble(2);
    if (Mth::abs(xd) > 10.0) xd = 0;
    if (Mth::abs(yd) > 10.0) yd = 0;
    if (Mth::abs(zd) > 10.0) zd = 0;

    double xx = pos->getDouble(0);
    double yy = pos->getDouble(1);
    double zz = pos->getDouble(2);

    xo = xOld = x = xx;
    yo = yOld = y = yy;
    zo = zOld = z = zz;

    yRotO = yRot = fmod(rotation->getFloat(0), 360.0f);
    xRotO = xRot = fmod(rotation->getFloat(1), 360.0f);

    fallDistance = tag->getFloat("FallDistance");
    onFire = tag->getShort("Fire");
    airSupply = tag->getShort("Air");
    onGround = tag->getBoolean("OnGround");

    setPos(x, y, z);
    readAdditionalSaveData(tag);
    return (tag->errorState == 0);
}

ItemEntity* Entity::spawnAtLocation(int resource, int count) {
	return spawnAtLocation(resource, count, 0);
}
ItemEntity* Entity::spawnAtLocation(int resource, int count, float yOffs) {
	return spawnAtLocation(new ItemInstance(resource, count, 0), yOffs);
}
ItemEntity* Entity::spawnAtLocation(ItemInstance* itemInstance, float yOffs) {
	ItemEntity* ie = new ItemEntity(level, (float)x, (float)(y + yOffs), (float)z, *itemInstance);
	delete itemInstance;
	ie->throwTime = 10;
	level->addEntity(ie);
	return ie;
}
bool Entity::isOnFire() { return onFire > 0; }
bool Entity::interactPreventDefault() { return false; }
bool Entity::isItemEntity() { return false; }
bool Entity::isHangingEntity() { return false; }
int Entity::getAuxData() { return 0; }
void Entity::playStepSound(int xt, int yt, int zt, int t) {
    const Tile::SoundType* soundType = Tile::tiles[t]->soundType;
    if (level->getTile(xt, yt+1, zt) == Tile::topSnow->id) {
        soundType = Tile::topSnow->soundType;
        level->playSound(this, soundType->getStepSound(), soundType->getVolume() * 0.25f, soundType->getPitch());
    } else if (!Tile::tiles[t]->material->isLiquid()) {
        level->playSound(this, soundType->getStepSound(), soundType->getVolume() * 0.25f, soundType->getPitch());
    }
}
