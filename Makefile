OPT = -g

MODULES = ../Modules

RGB_DIR = ../RGB_files

IMAGE_DIR = /usr/people/4Dgifts/iristools
IMAGE_LIB = $(RGB_DIR)/librgb_files.a $(IMAGE_DIR)/libimage/libimage.a

include $(LIB_DIRECTORY)/BIC_PL/Makefile.include
include $(LIB_DIRECTORY)/David/Makefile.include

INCLUDE = $(BIC_PL_INCLUDE) $(DLIB_INCLUDE)
LIBS = $(DLIB_LIBS) $(BIC_PL_LIBS)
LINT_LIBS = $(DLIB_LINT_LIBS) $(BIC_PL_LINT_LIBS)

roidis_to_display: roidis_to_display.o roidis_to_display.ln
	$(CC) $(LFLAGS) roidis_to_display.o $(LIBS) -o $@
	lint -u $(LINTFLAGS) roidis_to_display.ln $(LINT_LIBS)
                    

###########################################################################

dump_mesh: dump_mesh.o
	$(CC) $(LFLAGS) dump_mesh.o $(LIBS) $(MODULES)/libgeometry.a -o $@

lint_dump_mesh: dump_mesh.ln
	lint -u $(LINTFLAGS) dump_mesh.ln \
          $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

make_edges: make_edges.o
	$(CC) $(LFLAGS) make_edges.o $(LIBS) -o $@

lint_make_edges: make_edges.ln
	lint -u $(LINTFLAGS) make_edges.ln $(LINT_LIBS)

###########################################################################

make_edge_points: make_edge_points.o
	$(CC) $(LFLAGS) make_edge_points.o $(LIBS) -o $@

lint_make_edge_points: make_edge_points.ln
	lint -u $(LINTFLAGS) make_edge_points.ln $(LINT_LIBS)

###########################################################################

make_gradient: make_gradient.o
	$(CC) $(LFLAGS) make_gradient.o $(LIBS) -o $@

lint_make_gradient: make_gradient.ln
	lint -u $(LINTFLAGS) make_gradient.ln $(LINT_LIBS)

###########################################################################

make_gradient_arrows: make_gradient_arrows.o
	$(CC) $(LFLAGS) make_gradient_arrows.o $(LIBS) -o $@

lint_make_gradient_arrows: make_gradient_arrows.ln
	lint -u $(LINTFLAGS) make_gradient_arrows.ln $(LINT_LIBS)

###########################################################################

zoom_gradient: zoom_gradient.o
	$(CC) $(LFLAGS) zoom_gradient.o $(LIBS) -o $@

lint_zoom_gradient: zoom_gradient.ln
	lint -u $(LINTFLAGS) zoom_gradient.ln $(LINT_LIBS)

###########################################################################

flip_activity: flip_activity.o
	$(CC) $(LFLAGS) flip_activity.o $(LIBS) -o $@

lint_flip_activity: flip_activity.ln
	lint -u $(LINTFLAGS) flip_activity.ln $(LINT_LIBS)

###########################################################################

create_sphere: create_sphere.o
	$(CC) $(LFLAGS) create_sphere.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_create_sphere: create_sphere.ln
	lint -u $(LINTFLAGS) create_sphere.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

create_ellipse: create_ellipse.o
	$(CC) $(LFLAGS) create_ellipse.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_create_ellipse: create_ellipse.ln
	lint -u $(LINTFLAGS) create_ellipse.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

create_tetra: create_tetra.o
	$(CC) $(LFLAGS) create_tetra.o \
             $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_create_tetra: create_tetra.ln
	lint -u $(LINTFLAGS) create_tetra.ln $(LINT_LIBS) \
                    $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

subdivide_polygons: read_write.o subdivide_polygons.o
	$(CC) $(LFLAGS) read_write.o subdivide_polygons.o \
                  $(MODULES)/libgeometry.a $(MODULES)/libdata_structures.a \
                  $(LIBS) -o $@

lint_subdivide_polygons: read_write.ln subdivide_polygons.ln
	lint -u $(LINTFLAGS) read_write.ln subdivide_polygons.ln \
                $(MODULES)/lint/llib-lgeometry.ln \
                $(MODULES)/lint/llib-ldata_structures.ln \
                $(LINT_LIBS)

###########################################################################

make_normals: read_write.o make_normals.o
	$(CC) $(LFLAGS) read_write.o make_normals.o $(LIBS) -o $@

lint_make_normals: read_write.ln make_normals.ln
	lint -u $(LINTFLAGS) read_write.ln make_normals.ln $(LINT_LIBS)

###########################################################################

half_polygons: read_write.o half_polygons.o
	$(CC) $(LFLAGS) read_write.o half_polygons.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_half_polygons: read_write.ln half_polygons.ln
	lint -u $(LINTFLAGS) read_write.ln half_polygons.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

deform_surface: deform_surface.o
	$(CC) $(LFLAGS) deform_surface.o $(MODULES)/libdeform.a $(MODULES)/libgeometry.a $(MODULES)/libdata_structures.a $(LIBS) -o $@

lint_deform_surface: deform_surface.ln
	lint -u $(LINTFLAGS) deform_surface.ln \
          $(MODULES)/lint/llib-ldeform.ln $(MODULES)/lint/llib-lgeometry.ln \
          $(MODULES)/lint/llib-ldata_structures.ln \
          $(LINT_LIBS)

###########################################################################

transform_objects: transform_objects.o
	$(CC) $(LFLAGS) transform_objects.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_transform_objects: transform_objects.ln
	lint -u $(LINTFLAGS) transform_objects.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

rotate_objects: rotate_objects.o
	$(CC) $(LFLAGS) rotate_objects.o $(LIBS) -o $@

lint_rotate_objects: rotate_objects.ln
	lint -u $(LINTFLAGS) rotate_objects.ln $(LINT_LIBS)

###########################################################################

transform_to_world: transform_to_world.o
	$(CC) $(LFLAGS) transform_to_world.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_transform_to_world: transform_to_world.ln
	lint -u $(LINTFLAGS) transform_to_world.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

evaluate: evaluate.o
	$(CC) $(LFLAGS) evaluate.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_evaluate: evaluate.ln
	lint -u $(LINTFLAGS) evaluate.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

voxelate_landmarks: voxelate_landmarks.o
	$(CC) $(LFLAGS) voxelate_landmarks.o $(LIBS) -o $@

lint_voxelate_landmarks: voxelate_landmarks.ln
	lint -u $(LINTFLAGS) voxelate_landmarks.ln \
            $(LINT_LIBS)

###########################################################################

fit_line: fit_line.o
	$(CC) $(LFLAGS) fit_line.o $(LIBS) -o $@

lint_fit_line: fit_line.ln
	lint -u $(LINTFLAGS) fit_line.ln $(LINT_LIBS)

###########################################################################

copy_colours: copy_colours.o
	$(CC) $(LFLAGS) copy_colours.o $(LIBS) -o $@

lint_copy_colours: copy_colours.ln
	lint -u $(LINTFLAGS) copy_colours.ln $(LINT_LIBS)

###########################################################################

to_ascii: to_ascii.o
	$(CC) $(LFLAGS) to_ascii.o $(LIBS) -o $@

lint_to_ascii: to_ascii.ln
	lint -u $(LINTFLAGS) to_ascii.ln $(LINT_LIBS)

###########################################################################

make_labels: make_labels.o
	$(CC) $(LFLAGS) make_labels.o $(LIBS) -o $@

lint_make_labels: make_labels.ln
	lint -u $(LINTFLAGS) make_labels.ln $(LINT_LIBS)

###########################################################################

examine_polygons: examine_polygons.o
	$(CC) $(LFLAGS) examine_polygons.o $(LIBS) -o $@

lint_examine_polygons: examine_polygons.ln
	lint -u $(LINTFLAGS) examine_polygons.ln  \
                     $(LINT_LIBS)


###########################################################################

create_landmark_volume: create_landmark_volume.o
	$(CC) $(LFLAGS) create_landmark_volume.o $(LIBS) -o $@

lint_create_landmark_volume: create_landmark_volume.ln
	lint -u $(LINTFLAGS) create_landmark_volume.ln  \
                     $(LINT_LIBS)

###########################################################################

create_landmark_full_volume: create_landmark_full_volume.o get_filenames.o
	$(CC) $(LFLAGS) create_landmark_full_volume.o get_filenames.o $(LIBS) -o $@

lint_create_landmark_full_volume: create_landmark_full_volume.ln get_filenames.ln
	lint -u $(LINTFLAGS) create_landmark_full_volume.ln get_filenames.ln  \
                     $(LINT_LIBS)

###########################################################################

colour_curvature: colour_curvature.o
	$(CC) $(LFLAGS) colour_curvature.o $(MODULES)/libgeometry.a \
                    $(LIBS) -o $@

lint_colour_curvature: colour_curvature.ln
	lint -u $(LINTFLAGS) colour_curvature.ln \
          $(MODULES)/lint/llib-lgeometry.ln \
          $(LINT_LIBS)


###########################################################################

extract_points: extract_points.o
	$(CC) $(LFLAGS) extract_points.o $(LIBS) -o $@

lint_extract_points: extract_points.ln
	lint -u $(LINTFLAGS) extract_points.ln $(LINT_LIBS)

###########################################################################

dilate: dilate.o
	$(CC) $(LFLAGS) dilate.o $(LIBS) -o $@

lint_dilate: dilate.ln
	lint -u $(LINTFLAGS) dilate.ln $(LINT_LIBS)


###########################################################################

left_minus_right: left_minus_right.o
	$(CC) $(LFLAGS) left_minus_right.o $(LIBS) -o $@

lint_left_minus_right: left_minus_right.ln
	lint -u $(LINTFLAGS) left_minus_right.ln $(LINT_LIBS)

###########################################################################

count_tags: count_tags.o
	$(CC) $(LFLAGS) count_tags.o $(LIBS) -o $@

lint_count_tags: count_tags.ln
	lint -u $(LINTFLAGS) count_tags.ln $(LINT_LIBS)

###########################################################################

count_region: count_region.o
	$(CC) $(LFLAGS) count_region.o $(LIBS) -o $@

lint_count_region: count_region.ln
	lint -u $(LINTFLAGS) count_region.ln $(LINT_LIBS)


###########################################################################

create_volume: create_volume.o
	$(CC) $(LFLAGS) create_volume.o $(LIBS) -o $@

lint_create_volume: create_volume.ln
	lint -u $(LINTFLAGS) create_volume.ln $(LINT_LIBS)


###########################################################################

transform_tags: transform_tags.o
	$(CC) $(LFLAGS) transform_tags.o $(LIBS) -o $@

lint_transform_tags: transform_tags.ln
	lint -u $(LINTFLAGS) transform_tags.ln $(LINT_LIBS)


###########################################################################

tag_statistics: tag_statistics.o
	$(CC) $(LFLAGS) tag_statistics.o $(MODULES)/libnumerical.a $(LIBS) \
               -o $@

lint_tag_statistics: tag_statistics.ln
	lint -u $(LINTFLAGS) tag_statistics.ln $(MODULES)/lint/llib-lnumerical.ln \
              $(LINT_LIBS)


###########################################################################

remove_singles: remove_singles.o
	$(CC) $(LFLAGS) remove_singles.o $(LIBS) -o $@

lint_remove_singles: remove_singles.ln
	lint -u $(LINTFLAGS) remove_singles.ln $(LINT_LIBS)


###########################################################################

frequencies: frequencies.o
	$(CC) $(LFLAGS) frequencies.o $(LIBS) -o $@

lint_frequencies: frequencies.ln
	lint -u $(LINTFLAGS) frequencies.ln $(LINT_LIBS)


###########################################################################

matrix1: matrix1.o
	$(CC) $(LFLAGS) matrix1.o $(LIBS) -o $@

lint_matrix1: matrix1.ln
	lint -u $(LINTFLAGS) matrix1.ln $(LINT_LIBS)


###########################################################################

check_tags: check_tags.o
	$(CC) $(LFLAGS) check_tags.o $(LIBS) -o $@

lint_check_tags: check_tags.ln
	lint -u $(LINTFLAGS) check_tags.ln $(LINT_LIBS)


###########################################################################

find_interruptions: find_interruptions.o
	$(CC) $(LFLAGS) find_interruptions.o $(LIBS) -o $@

lint_find_interruptions: find_interruptions.ln
	lint -u $(LINTFLAGS) find_interruptions.ln $(LINT_LIBS)


###########################################################################

surface_mask: surface_mask.o
	$(CC) $(LFLAGS) surface_mask.o $(LIBS) -o $@

lint_surface_mask: surface_mask.ln
	lint -u $(LINTFLAGS) surface_mask.ln $(LINT_LIBS)


###########################################################################

tag_mask: tag_mask.o
	$(CC) $(LFLAGS) tag_mask.o $(LIBS) -o $@

lint_tag_mask: tag_mask.ln
	lint -u $(LINTFLAGS) tag_mask.ln $(LINT_LIBS)

###########################################################################

add_surfaces: add_surfaces.o
	$(CC) $(LFLAGS) add_surfaces.o $(LIBS) -o $@

lint_add_surfaces: add_surfaces.ln
	lint -u $(LINTFLAGS) add_surfaces.ln $(LINT_LIBS)


###########################################################################

remove_tags: remove_tags.o
	$(CC) $(LFLAGS) remove_tags.o $(LIBS) -o $@

lint_remove_tags: remove_tags.ln
	lint -u $(LINTFLAGS) remove_tags.ln $(LINT_LIBS)


###########################################################################

diff_points: diff_points.o
	$(CC) $(LFLAGS) diff_points.o $(LIBS) -o $@

lint_diff_points: diff_points.ln
	lint -u $(LINTFLAGS) diff_points.ln $(LINT_LIBS)


###########################################################################

stats_tag_file: stats_tag_file.o
	$(CC) $(LFLAGS) stats_tag_file.o  $(MODULES)/libnumerical.a \
        $(LIBS) -o $@

lint_stats_tag_file: stats_tag_file.ln
	lint -u $(LINTFLAGS) stats_tag_file.ln  $(MODULES)/llib-lnumerical.ln $(LINT_LIBS)

###########################################################################

make_slice_volume: make_slice_volume.o
	$(CC) $(LFLAGS) make_slice_volume.o $(LIBS) -o $@

lint_make_slice_volume: make_slice_volume.ln
	lint -u $(LINTFLAGS) make_slice_volume.ln $(LINT_LIBS)

###########################################################################

create_ellipsoids: create_ellipsoids.o
	$(CC) $(LFLAGS) create_ellipsoids.o \
             $(MODULES)/libgeometry.a \
             $(LIBS) -o $@

lint_create_ellipsoids: create_ellipsoids.ln
	lint -u $(LINTFLAGS) create_ellipsoids.ln $(LINT_LIBS)

###########################################################################

volume_colour: volume_colour.o
	$(CC) $(LFLAGS) volume_colour.o $(LIBS) -o $@

lint_volume_colour: volume_colour.ln
	lint -u $(LINTFLAGS) volume_colour.ln $(LINT_LIBS)


###########################################################################

cards: cards.o
	$(CC) $(LFLAGS) cards.o $(LIBS) -o $@

lint_cards: cards.ln
	lint -u $(LINTFLAGS) cards.ln $(LINT_LIBS)

###########################################################################

lmk_to_tag: lmk_to_tag.o
	$(CC) $(LFLAGS) lmk_to_tag.o $(LIBS) -o $@

lint_lmk_to_tag: lmk_to_tag.ln
	lint -u $(LINTFLAGS) lmk_to_tag.ln $(LINT_LIBS)

###########################################################################

histogram_volume: histogram_volume.o
	$(CC) $(LFLAGS) histogram_volume.o $(LIBS) \
             $(MODULES)/libnumerical.a -o $@

lint_histogram_volume: histogram_volume.ln
	lint -u $(LINTFLAGS) histogram_volume.ln $(LINT_LIBS) \
             $(MODULES)/lint/llib-lnumerical.ln

###########################################################################

make_slice: make_slice.o
	$(CC) $(LFLAGS) make_slice.o $(LIBS) $(MODULES)/libgeometry.a -o $@

lint_make_slice: make_slice.ln
	lint -u $(LINTFLAGS) make_slice.ln \
          $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

test_map: test_map.o
	$(CC) $(LFLAGS) test_map.o $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_test_map: test_map.ln
	lint -u $(LINTFLAGS) test_map.ln \
          $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

basic_stats: basic_stats.o
	$(CC) $(LFLAGS) basic_stats.o $(MODULES)/libnumerical.a $(LIBS) \
               -o $@

lint_basic_stats: basic_stats.ln
	lint -u $(LINTFLAGS) basic_stats.ln $(MODULES)/lint/llib-lnumerical.ln \
              $(LINT_LIBS)

###########################################################################

find_vertex: find_vertex.o
	$(CC) $(LFLAGS) find_vertex.o  $(LIBS) -o $@

lint_find_vertex: find_vertex.ln
	lint -u $(LINTFLAGS) find_vertex.ln  $(LINT_LIBS)

###########################################################################

compare_volumes: compare_volumes.o
	$(CC) $(LFLAGS) compare_volumes.o  $(LIBS) -o $@

lint_compare_volumes: compare_volumes.ln
	lint -u $(LINTFLAGS) compare_volumes.ln  $(LINT_LIBS)

###########################################################################

extract_sulci: extract_sulci.o
	$(CC) $(LFLAGS) extract_sulci.o  $(LIBS) -o $@

lint_extract_sulci: extract_sulci.ln
	lint -u $(LINTFLAGS) extract_sulci.ln  $(LINT_LIBS)

###########################################################################

mask_volume: mask_volume.o
	$(CC) $(LFLAGS) mask_volume.o  $(LIBS) -o $@

lint_mask_volume: mask_volume.ln
	lint -u $(LINTFLAGS) mask_volume.ln  $(LINT_LIBS)

###########################################################################

vol2roi: vol2roi.o
	$(CC) $(LFLAGS) vol2roi.o  $(LIBS) -o $@

lint_vol2roi: vol2roi.ln
	lint -u $(LINTFLAGS) vol2roi.ln  $(LINT_LIBS)

###########################################################################

tubify: tubify.o
	$(CC) $(LFLAGS) tubify.o  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_tubify: tubify.ln
	lint -u $(LINTFLAGS) tubify.ln  $(MODULES)/lint/llib-lgeometry.ln \
             $(LINT_LIBS)

###########################################################################

polygon_map: polygon_map.o
	$(CC) $(LFLAGS) polygon_map.o \
               $(MODULES)/libgeometry.a $(MODULES)/libdata_structures.a \
                $(LIBS) -o $@

lint_polygon_map: polygon_map.ln
	lint -u $(LINTFLAGS) polygon_map.ln  $(MODULES)/lint/llib-lgeometry.ln \
              $(MODULES)/lint/llib-ldata_structures.ln \
              $(LINT_LIBS)

###########################################################################

polygons_to_lines: polygons_to_lines.o
	$(CC) $(LFLAGS) polygons_to_lines.o \
                $(LIBS) -o $@

lint_polygons_to_lines: polygons_to_lines.ln
	lint -u $(LINTFLAGS) polygons_to_lines.ln  $(LINT_LIBS)

###########################################################################

plane_polygon_intersect: plane_polygon_intersect.o
	$(CC) $(LFLAGS) plane_polygon_intersect.o \
               $(MODULES)/libgeometry.a  \
                $(LIBS) -o $@

lint_plane_polygon_intersect: plane_polygon_intersect.ln
	lint -u $(LINTFLAGS) plane_polygon_intersect.ln  \
              $(MODULES)/lint/llib-lgeometry.ln \
              $(LINT_LIBS)

###########################################################################

set_line_width: set_line_width.o
	$(CC) $(LFLAGS) set_line_width.o $(LIBS) -o $@

lint_set_line_width: set_line_width.ln
	lint -u $(LINTFLAGS) set_line_width.ln  $(LINT_LIBS)

###########################################################################

obj_to_rgb: obj_to_rgb.o
	$(CC) $(LFLAGS) obj_to_rgb.o $(IMAGE_LIB) $(LIBS) -o $@

lint_obj_to_rgb: obj_to_rgb.ln
	lint -u $(LINTFLAGS) obj_to_rgb.ln  $(LINT_LIBS)

###########################################################################

set_object_colour: set_object_colour.o
	$(CC) $(LFLAGS) set_object_colour.o $(LIBS) -o $@

lint_set_object_colour: set_object_colour.ln
	lint -u $(LINTFLAGS) set_object_colour.ln  $(LINT_LIBS)

###########################################################################

clip_tags: clip_tags.o
	$(CC) $(LFLAGS) clip_tags.o $(LIBS) -o $@

lint_clip_tags: clip_tags.ln
	lint -u $(LINTFLAGS) clip_tags.ln  $(LINT_LIBS)

###########################################################################

tagtominc: tagtominc.o
	$(CC) $(LFLAGS) tagtominc.o $(LIBS) -o $@

lint_tagtominc: tagtominc.ln
	lint -u $(LINTFLAGS) tagtominc.ln  $(LINT_LIBS)

###########################################################################

minctotag: minctotag.o
	$(CC) $(LFLAGS) minctotag.o $(LIBS) -o $@

lint_minctotag: minctotag.ln
	lint -u $(LINTFLAGS) minctotag.ln  $(LINT_LIBS)

###########################################################################

average_voxels: average_voxels.o
	$(CC) $(LFLAGS) average_voxels.o $(LIBS) -o $@

lint_average_voxels: average_voxels.ln
	lint -u $(LINTFLAGS) average_voxels.ln  $(LINT_LIBS)

###########################################################################

change_image_colour: change_image_colour.o
	$(CC) $(LFLAGS) change_image_colour.o $(IMAGE_LIB) $(LIBS) -o $@

lint_change_image_colour: change_image_colour.ln
	lint -u $(LINTFLAGS) change_image_colour.ln  $(LINT_LIBS)

###########################################################################

dump_tal: dump_tal.o
	$(CC) $(LFLAGS) dump_tal.o $(LIBS) -o $@

lint_dump_tal: dump_tal.ln
	lint -u $(LINTFLAGS) dump_tal.ln  $(LINT_LIBS)

###########################################################################

test_arrays: test_arrays.o
	$(CC) $(LFLAGS) test_arrays.o $(LIBS) -o $@

lint_test_arrays: test_arrays.ln
	lint -u $(LINTFLAGS) test_arrays.ln  $(LINT_LIBS)

###########################################################################
