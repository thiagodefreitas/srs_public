/******************************************************************************
 * \file
 *
 * $Id:$
 *
 * Copyright (C) Brno University of Technology
 *
 * This file is part of software developed by dcgm-robotics@FIT group.
 *
 * Author: Vit Stancl (stancl@fit.vutbr.cz)
 * Supervised by: Michal Spanel (spanel@fit.vutbr.cz)
 * Date: 25/1/2012
 * 
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <but_server/octonode.h>

/**
 * Constructor
 */
octomap::EModelTreeNode::EModelTreeNode() :
	OcTreeNodeStamped(), m_r(255), m_g(255), m_b(255), m_a(255) {

}

/**
 * Destructor
 */
octomap::EModelTreeNode::~EModelTreeNode() {

}

/**
 * Create child
 */
bool octomap::EModelTreeNode::createChild(unsigned int i) {
	if (itsChildren == NULL) {
		allocChildren();
	}
	itsChildren[i] = new EModelTreeNode();
	return true;
}

void octomap::EModelTreeNode::updateColorChildren() {
	setAverageChildColor();
}

bool octomap::EModelTreeNode::pruneNode() {
	// checks for equal occupancy only, color ignored
	if (!this->collapsible())
		return false;
	// set occupancy value
	setLogOdds(getChild(0)->getLogOdds());
	// set color to average color
	if (isColorSet()) {
		setAverageChildColor();
	}
	// delete children
	for (unsigned int i = 0; i < 8; i++) {
		delete itsChildren[i];
	}
	delete[] itsChildren;
	itsChildren = NULL;
	return true;
}

void octomap::EModelTreeNode::expandNode() {
	assert(!hasChildren());

	// expand node, set children color same as node color
	for (unsigned int k = 0; k < 8; k++) {
		createChild(k);
		itsChildren[k]->setValue(value);
		getChild(k)->setColor(r(), g(), b(), a());
	}
}

void octomap::EModelTreeNode::setAverageChildColor() {
	int mr(0), mg(0), mb(0), ma(0);
	int c(0);
	for (int i = 0; i < 8; i++) {
		if (childExists(i) && getChild(i)->isColorSet()) {
			mr += getChild(i)->r();
			mg += getChild(i)->g();
			mb += getChild(i)->b();
			ma += getChild(i)->a();
			++c;
		}
	}
	if (c) {
		mr /= c;
		mg /= c;
		mb /= c;
		ma /= c;

		m_r = mr;
		m_g = mg;
		m_b = mb;
		m_a = ma;
		setColor((unsigned char) mr, (unsigned char) mg, (unsigned char) mb,
				(unsigned char) ma);
	} else { // no child had a color other than white
		setColor((unsigned char) 255, (unsigned char) 255, (unsigned char) 255,
				(unsigned char) 255);
	}
}

/**
 * Creates a new (empty) OcTree of a given resolution
 * @param _resolution
 */
octomap::EMOcTree::EMOcTree(double _resolution) :
	OccupancyOcTreeBase<octomap::EModelTreeNode> (_resolution) {
	itsRoot = new EModelTreeNode();
	tree_size++;
}

/**
 * Reads an OcTree from a binary file
 * @param _filename
 *
 */
octomap::EMOcTree::EMOcTree(std::string _filename) :
	OccupancyOcTreeBase<octomap::EModelTreeNode> (0.1) { // resolution will be set according to tree file
	itsRoot = new EModelTreeNode();
	tree_size++;

	readBinary(_filename);
}

octomap::EModelTreeNode* octomap::EMOcTree::setNodeColor(const OcTreeKey& key,
		const unsigned char& r, const unsigned char& g, const unsigned char& b,
		const unsigned char& a) {
	EModelTreeNode* n = search(key);
	if (n != 0) {
		n->setColor(r, g, b, a);
	}
	return n;
}

octomap::EModelTreeNode* octomap::EMOcTree::averageNodeColor(
		const OcTreeKey& key, const unsigned char& r, const unsigned char& g,
		const unsigned char& b, const unsigned char& a) {
	EModelTreeNode* n = search(key);
	if (n != 0) {
		if (n->isColorSet()) {
			// get previous color
			unsigned char prev_color_r = n->r();
			unsigned char prev_color_g = n->g();
			unsigned char prev_color_b = n->b();
			unsigned char prev_color_a = n->a();

			// average it with new color and set
			n->setColor((prev_color_r + r) / 2, (prev_color_g + g) / 2,
					(prev_color_b + b) / 2, (prev_color_a + a) / 2);
		} else {
			// nothing to average with
			n->setColor(r, g, b, a);
		}
	}
	return n;
}

octomap::EModelTreeNode* octomap::EMOcTree::integrateNodeColor(
		const OcTreeKey& key, const unsigned char& r, const unsigned char& g,
		const unsigned char& b, const unsigned char& a) {
	EModelTreeNode* n = search(key);
	if (n != 0) {
		if (n->isColorSet()) {
			// get previous color
			unsigned char prev_color_r = n->r();
			unsigned char prev_color_g = n->g();
			unsigned char prev_color_b = n->b();
			unsigned char prev_color_a = n->a();

			// average it with new color taking account of node probability
			double node_prob = n->getOccupancy();
			unsigned char new_r = (unsigned char) ((double) prev_color_r
					* node_prob + (double) r * (0.99 - node_prob));
			unsigned char new_g = (unsigned char) ((double) prev_color_g
					* node_prob + (double) g * (0.99 - node_prob));
			unsigned char new_b = (unsigned char) ((double) prev_color_b
					* node_prob + (double) b * (0.99 - node_prob));
			unsigned char new_a = (unsigned char) ((double) prev_color_a
					* node_prob + (double) a * (0.99 - node_prob));
			n->setColor(new_r, new_g, new_b, new_a);
		} else {
			n->setColor(r, g, b, a);
		}
	}
	return n;
}

void octomap::EMOcTree::updateInnerOccupancy() {
	this->updateInnerOccupancyRecurs(this->itsRoot, 0);
}

void octomap::EMOcTree::updateInnerOccupancyRecurs(EModelTreeNode* node,
		unsigned int depth) {
	// only recurse and update for inner nodes:
	if (node->hasChildren()) {
		// return early for last level:
		if (depth < this->tree_depth) {
			for (unsigned int i = 0; i < 8; i++) {
				if (node->childExists(i)) {
					updateInnerOccupancyRecurs(node->getChild(i), depth + 1);
				}
			}
		}
		node->updateOccupancyChildren();
		node->updateColorChildren();
	}
}

unsigned int octomap::EMOcTree::getLastUpdateTime() {
	// this value is updated whenever inner nodes are
	// updated using updateOccupancyChildren()
	return itsRoot->getTimestamp();
}

void octomap::EMOcTree::degradeOutdatedNodes(unsigned int time_thres) {
	unsigned int query_time = (unsigned int) time(NULL);

	for (leaf_iterator it = this->begin_leafs(), end = this->end_leafs(); it
			!= end; ++it) {
		if (this->isNodeOccupied(*it) && ((query_time - it->getTimestamp())
				> time_thres)) {
			integrateMissNoTime(&*it);
		}
	}
}

void octomap::EMOcTree::updateNodeLogOdds(EModelTreeNode* node,
		const float& update) const {
	OccupancyOcTreeBase<EModelTreeNode>::updateNodeLogOdds(node, update);
	node->updateTimestamp();
}

void octomap::EMOcTree::integrateMissNoTime(EModelTreeNode* node) const {
	OccupancyOcTreeBase<EModelTreeNode>::updateNodeLogOdds(node, probMissLog);
}

void octomap::EMOcTree::insertColoredScan(const typePointCloud& coloredScan,
		const octomap::point3d& sensor_origin, double maxrange, bool pruning,
		bool lazy_eval) {

	// convert colored scan to octomap pcl
	octomap::Pointcloud scan;
	octomap::pointcloudPCLToOctomap(coloredScan, scan);

	// insert octomap pcl to count new probabilities
	KeySet free_cells, occupied_cells;
	computeUpdate(scan, sensor_origin, free_cells, occupied_cells, maxrange);

	// insert data into tree  -----------------------
	for (KeySet::iterator it = free_cells.begin(); it != free_cells.end(); ++it) {
		updateNode(*it, false, lazy_eval);
	}
	for (KeySet::iterator it = occupied_cells.begin(); it
			!= occupied_cells.end(); ++it) {
		updateNode(*it, true, lazy_eval);
	}

	// TODO: does pruning make sense if we used "lazy_eval"?
	if (pruning)
		this->prune();

	// update node colors
	BOOST_FOREACH (const pcl::PointXYZRGB& pt, coloredScan.points)
	{
		// averageNodeColor or integrateNodeColor can be used to count final color
		averageNodeColor(pt.x, pt.y, pt.z, (unsigned char)pt.r, (unsigned char)pt.g, (unsigned char)pt.b, (unsigned char)255);
	}

}