#include <gtest/gtest.h>

#include "write-manager.h"

class WriteManagerTest : public ::testing::Test {
protected:
	WriteManager *manager = nullptr;

	void SetUp() override { manager = write_manager_create(); }
	void TearDown() override
	{
		if (manager)
			write_manager_destroy(manager);
	}
};

TEST_F(WriteManagerTest, StoresVideoInfoFromUpdateSource)
{
	VideoInfo info{1920, 1080, PIXEL_FORMAT_NV12};
	write_manager_update_source(manager, &info);

	EXPECT_EQ(manager->video_info.width, 1920u);
	EXPECT_EQ(manager->video_info.height, 1080u);
	EXPECT_EQ(manager->video_info.format, PIXEL_FORMAT_NV12);
}

TEST_F(WriteManagerTest, ReplacesVideoInfoOnSourceUpdate)
{
	VideoInfo first{1920, 1080, PIXEL_FORMAT_NV12};
	write_manager_update_source(manager, &first);

	VideoInfo second{1280, 720, PIXEL_FORMAT_I420};
	write_manager_update_source(manager, &second);

	EXPECT_EQ(manager->video_info.width, 1280u);
	EXPECT_EQ(manager->video_info.height, 720u);
	EXPECT_EQ(manager->video_info.format, PIXEL_FORMAT_I420);
}
